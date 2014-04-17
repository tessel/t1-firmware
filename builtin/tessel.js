var util = require('util');
var EventEmitter = require('events').EventEmitter;

var tm = process.binding('tm');
var hw = process.binding('hw');

var tessel_version = process.versions.tessel_board;

var _interruptModes = {
  0 : "rise",
  1 : "fall",
  2 : "high",
  3 : "low",
  4 : "change"
};

process.send = function (msg) {
  hw.usb_send('M'.charCodeAt(0), JSON.stringify(msg));
}


/**
 * util
 */

function propertySetWithDefault (provided, possibility, defaultProp) {
  if (!provided) {
    return defaultProp;
  } 
  else {
    if (verifyParams(possibility, provided)) {
      return provided;
    } else {
      throw new Error("Invalid property value");
    }
  }
}

function verifyParams (possible, provided) {
  for (var obj in possible) {
    if (possible[obj] == provided) {
      return true;
    }
  }
  return false;
}

function intBytes (num) {
  var NUM_INT_BYTES = 4;
  var NUM_BITS_INT = 8;
  var buf = [];

  // Iterate through each octet
  for (var i = 0; i < NUM_INT_BYTES; i++) {
    // Grab the bytes at each octet and move it back to the beginning of the byte array
    var bite = (num & (0xFF << (i* NUM_BITS_INT))) >> (i * NUM_BITS_INT);

    // If the byte is still less than zero (usually the last octet of neg. number)
    if (bite < 0) {
      // Grab the last octet
      bite = bite & 0xFF
    }

    // Push the byte into the array
    buf.push(bite);
  }


  // Remove trailing zeros
  while (buf[--i] == 0) {
    // Pop that zero
    buf.pop(i);
  }

  return buf;
}


/**
 * Pins
 */

function Pin (pin) {
  this.pin = pin;
  this.pinmode = null;
  this.value = 0;
}

util.inherits(Pin, EventEmitter);

Pin.prototype.type = 'digital'

Pin.prototype.setInput = function (next) {
  hw.digital_input(this.pin);
  next && setImmediate(next);
  return this;
}

Pin.prototype.setOutput = function (initial, next) {
  if (typeof initial == 'function') {
    next = initial;
    initial = null;
  }
  hw.digital_write(this.pin, initial || initial != 'low' ? hw.HIGH : hw.LOW);
  hw.digital_output(this.pin);
  next && setImmediate(next);
  return this;
}

Pin.prototype.read = function (next) {
  var self = this;
  setImmediate(function () {
    hw.digital_input(self.pin);
    next && next(null, hw.digital_read(self.pin));
  });

  return this;
}

Pin.prototype.readSync = function () {
  hw.digital_input(this.pin);
  return hw.digital_read(this.pin);
}

Pin.prototype.write = function (value, next) {
  var self = this;
  setImmediate(function () {
    hw.digital_output(self.pin);
    hw.digital_write(self.pin, value ? hw.HIGH : hw.LOW);
    self.value = v;
    next && next();
  });

  return this;
}

Pin.prototype.writeSync = function (value) {
  hw.digital_output(this.pin);
  hw.digital_write(this.pin, value ? hw.HIGH : hw.LOW);
  this.value = v;
}

// TM <<<



Pin.prototype.mode = function (mode) {
  // truthy = input, falsy = output
  if (mode) {
    this.pinmode = 'input';
    hw.digital_input(this.pin);
  } else {
    this.pinmode = 'output';
    hw.digital_output(this.pin);
  }
  return this;
}

Pin.prototype.input = function () {
  return this.mode(true);
}

Pin.prototype.output = function () {
  return this.mode(false);
}

Pin.prototype.high = function () {
  return this.set(1);
}

Pin.prototype.low = function () {
  return this.set(0);
}

Pin.prototype.pulse = function () {
  this.high();
  this.low();
}

Pin.prototype.toggle = function () {
  return this.set(!this.value);
}

Pin.prototype.set = function (v) {
  // truthy = high, falsy = low
  hw.digital_write(this.pin, v ? hw.HIGH : hw.LOW);
  this.value = v;
  return this;
}

Pin.prototype.pwm = function (frequency, dutyCycle)
{
  // frequency is in hertz, on time is a percent
  var period = 1/(frequency/180000000);

  if (dutyCycle > 1) dutyCycle = 1;
  if (dutyCycle < 0) dutyCycle = 0;

  var pulsewidth = dutyCycle*period;

  if (hw.pwm(this.pin, hw.PWM_EDGE_HI, period, pulsewidth) != 0) {
    throw new Error("PWM is not supported on this pin");
  }
}


/**
 * Analog pins
 */

function AnalogPin (pin) {
  this.pin = pin;
  this.pinmode = 'input';
}

AnalogPin.prototype.write = function (val){
  if (tessel_version <= 2 && this.pin != hw.PIN_E_A4) {
    return console.log("Only A4 can write analog signals");
  } else if (tessel_version == 3 && this.pin != hw.PIN_E_A1){
    return console.log("Only A1 can write analog signals");
  }

  if (val < 0 || val > 1024) {
    return console.log("Analog values must be between 0 and 1024");
  }

  hw.analog_write(this.pin, val);
}


// TM 2014-01-30 new API >>>
var ANALOG_RESOLUTION = 1024;
AnalogPin.prototype.type = 'analog';
AnalogPin.prototype.resolution = ANALOG_RESOLUTION;
AnalogPin.prototype.readSync = function () {
  return hw.analog_read(this.pin) / ANALOG_RESOLUTION;
}
AnalogPin.prototype.read = function (next) {
  if (next) {
    setImmediate(next, null, hw.analog_read(this.pin) / ANALOG_RESOLUTION);
    return;
  }
  // TODO deprecated:
  return hw.analog_read(this.pin);
}
// TM <<<


/**
 * Interrupts
 */

// Event for GPIO interrupts
process.on('interrupt', function (interruptData) { 
  // Grab the interrupt generating message
  var index = parseInt(interruptData.interrupt, 10);
  var mode = _interruptModes[interruptData.mode];
  var state = parseInt(interruptData.state, 10);
  var time = parseInt(interruptData.time, 10);

  // Grab corresponding pin
  var assigned = board.interrupts[index];

  // If it exists and nothing went crazy
  if (assigned) {

    // Check if there are callbacks assigned for this mode or "change"
    var callbacks = assigned.modes[mode].callbacks;

    // If this is a rise or fall event, call change callbacks as well
    if (mode === "change") {

      if (state === 1) {
        callbacks.concat(assigned.modes['rise'].callbacks);
        assigned.pin.emit('change', null, time, 'low');
        assigned.pin.emit('rise', null, time, 'rise');
      } 
      else {
        callbacks.concat(assigned.modes['low'].callbacks);
        assigned.pin.emit('change', null, time, 'low');
        assigned.pin.emit('low', null, time, 'low');
      }
    }
    else {
      assigned.pin.emit(mode)
    }
    // If there are, call them
    for (var i = 0; i < callbacks.length; i++) {
      callbacks[i].bind(assigned.pin, null, time, mode)();
    }
        // If it's a high or low event, clear that interrupt (should only happen once)
    if (mode == 'high' || mode == 'low') {
      assigned.pin.cancelWatch();
    }
  }
});

Pin.prototype.watch = function(mode, callback) {

  // Make sure trigger mode is valid
  mode = mode.toLowerCase();
  if (interruptModeForType(mode) == -1) {
    return callback && callback.bind(this, new Error("Invalid trigger: " + mode))();
  }
  
  // Check if this pin already has an interrupt associated with it
  var interrupt = hw.interrupt_assignment_query(this.pin);

  // If it does not have an assignment
  if (interrupt === -1) {

    // Attempt to acquire next available
    interrupt = hw.acquire_available_interrupt();

    if (interrupt === -1) {
      console.warn("There are no interrupts remaining...");
      // Throw an angry Error
      return callback && callback(new Error("All GPIO Interrupts are already assigned."));
    }

  }

  // Assign this pin with this trigger mode to the interrupt
  return this.assignInterrupt(mode, interrupt, callback);
}

function interruptModeForType(triggerMode) {
  for (var mode in _interruptModes) {
    if (_interruptModes[mode] === triggerMode) {
      return mode;
    }
  }
  return -1;
}

Pin.prototype.assignInterrupt = function(triggerMode, interrupt, callback) {
  
  // Start watching pin for edges
  var success = hw.interrupt_watch(this.pin, interrupt, interruptModeForType(triggerMode));

  // If there was no error
  if (success != -1) {
   
    // Set the assignment on the board object
    // Grab what the current assignment is
    var assignment = board.interrupts[interrupt];

    // If there is something assigned 
    if (assignment) {

      // If the watch is being removed
      if (triggerFlag == -1) {

        // Just remove the entity
        delete board.interrupts[interrupt];
      }
      // If we're adding the mode
      else {
        // Add it to the array
        assignment.addModeListener(triggerMode, callback);
      }
    }
    // If not, assign with new data structure
    else {
      // Create the object
      assignment = new InterruptAssignment(this, triggerMode, callback);

      // Shove it into the data structure
      board.interrupts[interrupt] = assignment;
    }
  } 
  // If not successful watching...
  else {

    // Throw the error in callback
    callback && callback.bind(this, new Error("Unable to wait for interrupts on pin"))();
    return -1;
  }

  return 1;
}

function InterruptAssignment(pin, mode, callback) {
  // Save the pin object
  this.pin = pin;

  // Create a modes obj with each possible mode
  this.modes = {};
  this.modes["rise"] = {listeners : 0, callbacks : []};
  this.modes["fall"] = {listeners : 0, callbacks : []};
  this.modes["change"] = {listeners : 0, callbacks : []};
  this.modes["high"] = {listeners : 0, callbacks : []};
  this.modes["low"] = {listeners : 0, callbacks : []};

  // Increment number of listeners
  this.modes[mode].listeners = 1;

  // Add the callback to the appropriate mode callback array
  if (callback) {
    this.modes[mode].callbacks.push(callback);
  }
}

InterruptAssignment.prototype.addModeListener = function(mode, callback) {
  // Grab the mode object
  var modeObj = this.modes[mode];

  // Increment number of listeners
  modeObj.listeners++;

  // If a callback was provided
  if (callback) {

    // Add it to the array
    modeObj.callbacks.push(callback);
  }
}

Pin.prototype.cancelWatch = function(mode, callback){

  if (!mode) {
    return callback && callback(new Error("Need to specify trigger Type to cancel."));
  }
  // Check which modes are enabled by grabbing this interrupt
  var interruptID = hw.interrupt_assignment_query(this.pin);

  // If this has an interrupt associated with it
  if (interruptID != -1) {

    // Grab the JS assignment
    var assignment = board.interrupts[interruptID];

    if (assignment) {
            
      delete board.interrupts[interruptID];

      // Watch with a -1 flag to tell it to stop watching
      var success = hw.interrupt_unwatch(interruptID);

      // If it went well
      if (success) {

        // Call callback with no error
        callback && callback();

        return 1;
      } 
      // If it didn't
      else {

        // Throw the error
        callback && callback(new Error("Unable to cancel interrupt..."));

        return -1;
      }
    }
  }
  else {
    callback && callback();
    return 1;
  }
}


/**
 * Button
 */

function Button(pin){
  this.pin = pin;
}

Button.prototype = new EventEmitter();

Button.prototype.record = function(){
  var inter = hw.interrupt_available();
  hw.interrupt_record(inter, this.pin);

  var l = this;
  var lastfall = 0, lastrise = 0;
  setInterval(function () {
    var rise = hw.interrupt_record_lastrise(inter)
      , fall = hw.interrupt_record_lastfall(inter);
    if (rise != lastrise) {
      l.emit('rise', rise, fall);
    }
    lastrise = rise;
    if (fall != lastfall) {
      l.emit('fall', rise, fall); 
    }
    lastfall = fall;
  }, 1);
  return l;
}

/**
 * Board
 */

//
// I2C
//

var i2c0initialized = false;
var i2c1initialized = false;

var I2CMode = {
  Master : hw.I2C_MASTER,
  Slave: hw.I2C_SLAVE,
  General: hw.I2C_GENERAL
}

function I2C (addr, mode, port) {
  this.addr = addr;
  this.i2c_port = port;
  if (mode == hw.I2C_SLAVE){
    this.mode = hw.I2C_SLAVE;
  } else {
    this.mode = hw.I2C_MASTER;
  }
}

I2C.prototype._initialize = function () {
  // if you give an address, we assume it'll be a slave
  if (!i2c0initialized && this.i2c_port == hw.I2C_0 ){
    hw.i2c_initialize(hw.I2C_0);
    hw.i2c_enable(hw.I2C_0, this.mode);
    i2c0initialized = true;
  } else if (!i2c1initialized && this.i2c_port == hw.I2C_1 ){
    hw.i2c_initialize(hw.I2C_1);
    hw.i2c_enable(hw.I2C_1, this.mode);
    i2c1initialized = true;
  }

  if (this.mode == hw.I2C_SLAVE){
    hw.i2c_set_slave_addr(this.i2c_port, this.addr);
  }
};

I2C.prototype.disable = function() {

  if (i2c0initialized && this.i2c_port == hw.I2C_0 ){
    hw.i2c_disable(hw.I2C_0);
    i2c0initialized = false;
  } else if (i2c1initialized && this.i2c_port == hw.I2C_1 ){
    hw.i2c_disable(hw.I2C_1);
    i2c1initialized = false;
  }
}

// DEPRECATED old way of invoking I2C initialization
I2C.prototype.initialize = function () { }

I2C.prototype.transferSync = function (data, rxbuf_len) {
  this._initialize();
  if (this.mode == hw.I2C_SLAVE) {
    // TODO should be i2c_slave_transfer_blocking
    return hw.i2c_slave_transfer(this.i2c_port, data, rxbuf_len);
  } else {
    // TODO should be i2c_master_transfer_blocking
    return hw.i2c_master_transfer(this.i2c_port, this.addr, data, rxbuf_len);
  }
};

I2C.prototype.sendSync = function (data) {
  this._initialize();
  if (this.mode == hw.I2C_SLAVE) {
    return hw.i2c_slave_send(this.i2c_port, data);
  } else {
    return hw.i2c_master_send(this.i2c_port, this.addr, data);
  }
};

I2C.prototype.receiveSync = function (buf_len) {
  this._initialize();
  if (this.mode == hw.I2C_SLAVE) {
    // TODO should be i2c_slave_transfer_blocking
    return hw.i2c_slave_receive(this.i2c_port, '', buf_len);
  } else {
    // TODO should be i2c_master_receive_blocking
    return hw.i2c_master_receive(this.i2c_port, this.addr, buf_len);
  }
};


I2C.prototype.transfer = function (txbuf, rxbuf_len, unused_rxbuf, fn)
{
  if (!fn) {
    fn = unused_rxbuf;
    unused_rxbuf = null;
  }

  var self = this;
  setImmediate(function() {
    var rxbuf = self.transferSync(txbuf, rxbuf_len);
    fn && fn(null, rxbuf);
  });
}


I2C.prototype.send = function (txbuf, fn)
{
  var self = this;
  setImmediate(function() {
    self.sendSync(txbuf);
    fn && fn(null);
  });
};


I2C.prototype.receive = function (buf_len, unused_rxbuf, fn)
{
  if (!fn) {
    fn = unused_rxbuf;
    unused_rxbuf = null;
  }

  var self = this;
  setImmediate(function() {
    var rxbuf = self.receiveSync(buf_len, unused_rxbuf);
    fn && fn(null, rxbuf);
  });
};

/**
 * UART
 */

var UARTParity = {
  None : hw.UART_PARITY_NONE,
  Odd : hw.UART_PARITY_ODD,
  Even : hw.UART_PARITY_EVEN,
  OneStick : hw.UART_PARITY_ONE_STICK,
  ZeroStick : hw.UART_PARITY_ZERO_STICK,
}

var UARTDataBits = {
  Five : hw.UART_DATABIT_5,
  Six : hw.UART_DATABIT_6,
  Seven : hw.UART_DATABIT_7,
  Eight : hw.UART_DATABIT_8,
}

var UARTStopBits = {
  One : hw.UART_STOPBIT_1,
  Two : hw.UART_STOPBIT_2,
}

function UART(params, port) {

  // Call Stream constructor
  this.readable = true;
  this.writable = true;

  if (!params) params = {};

  this.uartPort = port;

  // Default baudrate is 9600
  this.baudrate = (params.baudrate ? params.baudrate : 9600);

  // Default databits is eight
  this.dataBits = propertySetWithDefault(params.dataBits, UARTDataBits, UARTDataBits.Eight);

  // Default parity is none
  this.parity = propertySetWithDefault(params.parity, UARTParity, UARTParity.None);

  // Default stopbits is one
  this.stopBits = propertySetWithDefault(params.stopBits, UARTStopBits, UARTStopBits.One);

  // Initialize the port
  this.initialize();

  // When we get UART data
  board.on('uartReceive', function(port, data) {
    // If it's on this port
    if (port == this.uartPort) {
      // Emit the data
      this.emit('data', data);
    }
  }.bind(this));
}

util.inherits(UART, EventEmitter);
                                                  

UART.prototype.initialize = function(){
  hw.uart_initialize(this.uartPort, this.baudrate, this.dataBits, this.parity, this.stopBits);
};

UART.prototype.enable = function(){
  hw.uart_enable(this.uartPort);
};

UART.prototype.disable = function(){
  hw.uart_disable(this.uartPort);
};

UART.prototype.setBaudrate = function(baudrate){
  if (baudrate) {
    this.baudrate = baudrate;
    this.initialize();
    // hw.uart_initialize(this.uartPort, baudrate, this.dataBits, this.parity, this.stopBits); 
  } else {
    return this.baudrate;
  }
}

UART.prototype.write = function (txbuf) {
  // Send it off
  return hw.uart_send(this.uartPort, txbuf);
}

UART.prototype.checkReceiveFlag = function() {

  // Check if we've received anything on an interrupt
  while (hw.uart_check_receive_flag(this.uartPort)) {
    // Grab the latest from the UART ring buffer
    var data = hw.uart_receive(this.uartPort, 2048);

    if (data && data.length) {
      this.emit("data", data);
    }
  } 
};

UART.prototype.setDataBits = function(dataBits) {
  if (verifyParams(UARTDataBits, dataBits)) {
    this.dataBits = dataBits;
    this.initialize();
    return 1;
  }

  throw new Error("Invalid databits value");
  return -1;
};

UART.prototype.setParity = function(parity) {
  if (verifyParams(UARTParity, parity)) {
    this.parity = parity;
    this.initialize();
    return 1;
  }

  throw new Error("Invalid parity value");
  return -1;
};

UART.prototype.setStopBits = function(stopBits){
  if (verifyParams(UARTStopBits, stopBits)) {
    this.stopBits = stopBits;
    this.initialize();
    return 1;
  }

  throw new Error("Invalid stopbit value");
  return -1;
};

global._G.uart_receive_callback = function(port, data) {
  board.emit("uartReceive", port, data);
};


/**
 * SPI
 */

// SPI parameters may be changed by different invocations,
// cache which SPI was used most recently to update parameters
// only when necessary.

var _currentSPI = null;

function SPI (params)
{ 
  params = params || {};

  // Eventually get correct port for hardware
  this.port = hw.SPI_0;

  // accept dataMode or cpol/cpha
  if (typeof params.dataMode == 'number') {
    params.cpol = params.dataMode & 0x1;
    params.cpha = params.dataMode & 0x2;
  }
  this.cpol = params.cpol == 'high' || params.cpol == 1 ? 1 : 0;
  this.cpha = params.cpha == 'second' || params.cpha == 1 ? 1 : 0;

  // Speed of the clock
  this.clockSpeed = params.clockSpeed || 100000;

  // Frame Mode (by vendor)
  // TODO add other frame modes here ('ti', 'microwire')
  this.frameMode = 'normal';

  // Slave vs. Master
  this.role = params.role == 'slave' ? 'slave': 'master';

  // MSB vs LSB Needs testing
  this.bitOrder = propertySetWithDefault(params.bitOrder, SPIBitOrder, SPIBitOrder.MSB);

  // chip Select is user wants us to toggle it for them
  this.chipSelect = params.chipSelect || null;

  this.chipSelectActive = params.chipSelectActive == 'high' || params.chipSelectActive == 1 ? 1 : 0;

  if (this.chipSelect) {
    this.chipSelect.setOutput();
    if (this.chipSelectActive) {
      this.chipSelect.low();
    } else {
      this.chipSelect.high();
    }
  }

  // initialize here to pull sck, mosi, miso pins to default state
  this._initialize();
  
  // Confirmation that CS has been pulled high.
  var self = this;
  setImmediate(function () {
    self.emit('ready');
  })
}


util.inherits(SPI, EventEmitter);


SPI.prototype._activeChipSelect = function (flag)
{
  if (this.chipSelect) {
    if (this.chipSelectActive) {
      flag ? this.chipSelect.high() : this.chipSelect.low();
    } else {
      flag ? this.chipSelect.low() : this.chipSelect.high();
    }
  }
}


SPI.prototype._initialize = function ()
{
  if (_currentSPI != this) {
    hw.spi_initialize(this.port,
      Number(this.clockSpeed),
      this.role == 'slave' ? SPIMode.Slave : SPIMode.Master,
      this.cpol ? 1 : 0,
      this.cpha ? 1 : 0,
      this.frameMode == 'normal' ? SPIFrameMode.Normal : SPIFrameMode.Normal);
  }
}


// DEPRECATED this was the old way to invoke I2C initialize
SPI.prototype.initialize = function () { }


SPI.prototype.close = SPI.prototype.end = function ()
{
  hw.spi_disable(this.port);
};


SPI.prototype.transferSync = function (txbuf, unused_rxbuf)
{
  this._initialize();
  this._activeChipSelect(1);
  var rxbuf = hw.spi_transfer(this.port, txbuf);
  this._activeChipSelect(0);
  return rxbuf;
}


SPI.prototype.sendSync = function (txbuf)
{
  this._initialize();
  this._activeChipSelect(1);
  hw.spi_send(this.port, txbuf);
  this._activeChipSelect(0);
}


SPI.prototype.receiveSync = function (buf_len, unused_rxbuf)
{
  this._initialize();
  this._activeChipSelect(1);
  var rxbuf = hw.spi_receive(this.port, buf_len);
  this._activeChipSelect(0);
  return rxbuf;
}


SPI.prototype.transfer = function (txbuf, unused_rxbuf, fn)
{
  if (!fn) {
    fn = unused_rxbuf;
    unused_rxbuf = null;
  }

  var self = this;
  setImmediate(function() {
    // TODO: this needs to change to callback
    var rxbuf = hw.spi_transfer_async(txbuf, unused_rxbuf);
    fn && fn(null, rxbuf);
  });
}


SPI.prototype.send = function (txbuf, fn)
{
  var self = this;
  setImmediate(function() {
    hw.spi_send_async(txbuf);
    fn && fn(null);
  });
};


SPI.prototype.receive = function (buf_len, unused_rxbuf, fn)
{
  if (!fn) {
    fn = unused_rxbuf;
    unused_rxbuf = null;
  }

  var self = this;
  setImmediate(function() {
    var rxbuf = hw.spi_receive_async(buf_len, unused_rxbuf);
    fn && fn(null, rxbuf);
  });
};


SPI.prototype.setBitOrder = function (bitOrder)
{
  this.bitOrder = bitOrder;
}


SPI.prototype.setClockSpeed = function(clockSpeed) {
  if (clockSpeed > 0 && clockSpeed < 20e6) {
    this.clockSpeed = clockSpeed;
  } else {
    throw new Error("Invalid clockspeed value");
  }
}


SPI.prototype.setDataMode = function (dataMode)
{
  this.cpol = dataMode & 0x1;
  this.cpha = dataMode & 0x2;
}


SPI.prototype.setFrameMode = function (frameMode)
{
  this.frameMode = frameMode;
}


SPI.prototype.setRole = function (role)
{
  this.role = role;
}


SPI.prototype.setChipSelectMode = function (chipSelectMode)
{
  this.chipSelectMode = chipSelectMode;
  this.activeChipSelect(0);
}


var SPIBitOrder = {
  MSB : 0,
  LSB : 1,
}

/* 
*   MODE         CPOL   CPHA
*   SPI_MODE0      0     0
*   SPI_MODE1      0     1
*   SPI_MODE2      1     0
*   SPI_MODE3      1     1 
*/
var SPIDataMode = {
  Mode0 : hw.SPI_MODE_0, // should be mode 2
  Mode1 : hw.SPI_MODE_1, 
  Mode2 : hw.SPI_MODE_2, // should be mode 0
  Mode3 : hw.SPI_MODE_3,
};

var SPIMode = {
  Slave : hw.SPI_SLAVE_MODE,
  Master : hw.SPI_MASTER_MODE,
};

var SPIFrameMode = {
  Normal : hw.SPI_NORMAL_FRAME,
  TI : hw.SPI_TI_FRAME,
  MicroWire : hw.SPI_MICROWIRE_FRAME,
};

var SPIChipSelectMode = {
  ActiveHigh : 0,
  ActiveLow : 1,
};


/**
 * Ports
 */

function Port (id, gpios, analogs, i2c, uart)
{
  this.id = id;

  this._gpios = gpios;
  this._analogs = analogs || [];

  // TM 2014-01-30 new API >>>
  this.digital = gpios;
  for (var i = 0; i < analogs.length; i++) {
    this.analog[i] = analogs[i];
  }
  // TM <<<
  // Until new takes a "this" method
  this.I2C = function (addr, mode, port) {
    return new I2C(addr, mode, port == null ? i2c : port)
  };
  this.UART = function (format, port) {
    if (uart == null) {
      throw tessel_version > 1 ? 'Software UART not yet functional in firmware.' : 'Board version only supports UART on GPIO port.';
    }
    return new UART(format, port == null ? uart : port);
  };
};

// Port.prototype.I2C = function (addr, port) {
//     return new I2C(addr, port == null ? i2c : port)
//   };

// Port.prototype.UART = function (format, port) {
//     if (this._uart == null) {
//       throw 'Board version does not support UART on this port.';
//     }
//     return new UART(format, port == null ? uart : port);
//   };

Port.prototype.SPI = function (format, port){
  return new SPI(format);
};

Port.prototype.pinOutput = function (n) {
  hw.digital_output(n);
};

Port.prototype.digitalWrite = function (n, val) {
  hw.digital_write(n, val ? hw.HIGH : hw.LOW);
};

Port.prototype.gpio = function (n) {
  return this._gpios[n];
};

Port.prototype.analog = function (n) {
  return this._analogs[n];
};

function Tessel() {

  if (Tessel.instance) {
    return Tessel.instance;
  }
  else {
    Tessel.instance = this;
  }
  
  this.leds = [null, new Pin(hw.PIN_LED1), new Pin(hw.PIN_LED2), new Pin(hw.PIN_WIFI_ERR_LED), new Pin(hw.PIN_WIFI_CONN_LED)];
  this.led = function(n) {
    if (board.leds[n] == null) {
      throw "No LED at index " + n + " exists."
    }
    return board.leds[n];
  }
  this.interrupts = [];

  this.ports =  {
    A: new Port('A', [null, new Pin(hw.PIN_A_G1), new Pin(hw.PIN_A_G2), new Pin(hw.PIN_A_G3)], [],
      hw.I2C_1,
      tessel_version > 1 ? hw.UART_3 : null
    ),
    B: new Port('B', [null, new Pin(hw.PIN_B_G1), new Pin(hw.PIN_B_G2), new Pin(hw.PIN_B_G3)], [],
      hw.I2C_1,
      tessel_version > 1 ? hw.UART_2 : null
    ),
    C: new Port('C', [null, new Pin(hw.PIN_C_G1), new Pin(hw.PIN_C_G2), new Pin(hw.PIN_C_G3)], [],
      tessel_version > 1 ? hw.I2C_0 : hw.I2C_1,
      // tessel_version > 1 ? hw.UART_SW_0 : null
      null
    ),
    D: new Port('D', [null, new Pin(hw.PIN_D_G1), new Pin(hw.PIN_D_G2), new Pin(hw.PIN_D_G3)], [],
      tessel_version > 1 ? hw.I2C_0 : hw.I2C_1,
      tessel_version > 1 ? hw.UART_0 : null
    ),
    GPIO: new Port('GPIO', [null,
      new Pin(hw.PIN_E_G1), new Pin(hw.PIN_E_G2), new Pin(hw.PIN_E_G3),
      new Pin(hw.PIN_E_G4), new Pin(hw.PIN_E_G5), new Pin(hw.PIN_E_G6)
    ], [null,
      new AnalogPin(hw.PIN_E_A1), new AnalogPin(hw.PIN_E_A2), new AnalogPin(hw.PIN_E_A3),
      new AnalogPin(hw.PIN_E_A4), new AnalogPin(hw.PIN_E_A5), new AnalogPin(hw.PIN_E_A6)
    ],
      hw.I2C_1,
      tessel_version > 1 ? null : hw.UART_2
    ),
  };

  this.button = new Button(hw.BUTTON);

  this.port = function (label) {
    return board.ports[label.toUpperCase()];
  }

  this.sleep = function (n) {
    if (n < 1) {
      n = 1;
    }
    hw.sleep_ms(n);
  }

  this.deviceId = function(){
    return hw.device_id();
  }  

}

util.inherits(Tessel, EventEmitter);

var board = module.exports = new Tessel();

// TM 2014-01-30 new API >>>
for (var key in board.ports) {
  board.port[key] = board.ports[key];
}
board.led[1] = board.leds[1];
board.led[2] = board.leds[2];
board.led[3] = board.leds[3];
board.led[4] = board.leds[4];
// TM <<<

module.exports.I2CMode = I2CMode;
module.exports.SPIBitOrder = SPIBitOrder;
module.exports.SPIDataMode = SPIDataMode;
module.exports.SPIMode = SPIMode;
module.exports.SPIFrameMode = SPIFrameMode;
module.exports.SPIChipSelectMode = SPIChipSelectMode;
module.exports.UARTParity = UARTParity;
module.exports.UARTDataBits = UARTDataBits;
module.exports.UARTStopBits = UARTStopBits;