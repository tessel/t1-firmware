// GPIO bank vars
var tessel = require('tessel');
var gpio = tessel.port['GPIO'];
var pinOutput = gpio.pin['G5'];
var pinInput = gpio.pin['G3'];
var test = require('tape');

// Neopixel related vars
var Neopixels = require('neopixels');
var neopixels = new Neopixels();
var numNeopixels = 24;

// Percent diff of what's read and what's expected due to setTimeout inaccuracy
var marginOfError = 0.25;

// make sure type input checks work
test('testing input validation', function (t) {
  try {
    pinInput.readPulse(2,5000, function (err,pul) {});
  } catch(e) {
    t.equal(e.message, 'SCT input pulse type not set correctly. Must be either "high" or "low"', 'pulse type validation');
  }
  try {
    pinInput.readPulse('high','high', function (err,pul) {});
  } catch(e) {
    t.equal(e.message, 'SCT input pulse timeout not a number', 'pulse timeout type validation');
  }
  try {
    pinInput.readPulse('high',10001, function (err,pul) {});
  } catch(e) {
    t.equal(e.message, 'SCT input pulse timeout too large, must be less than 10000ms', 'pulse timeout bounds validation');
  }
  t.end();
});

// The main test
pinOutput.write(0);
test('testing readPulse functionality', testReadPulseThenNeopixel);

// Test if a high pulse can be read and that neopixel does not interfere it
function testReadPulseThenNeopixel(t) {
  pinInput.readPulse('high',5000, function (err,pul) {
    t.error(err,'no error when reading 250ms pulse');
    t.equal((Math.abs(250-pul)/250) <= marginOfError, true, 'high 250ms pulse');
  });
  neopixels.animate(numNeopixels, Buffer.concat(tracer(numNeopixels)), function (err) {
    t.equal(err == null,false,'error when animating neopixel');
    t.equal(err.message, 'SCT is already in use by Read Pulse', 'neopixel tried using sct (in use by readPulse)');
  });
  setTimeout( function () {
    pinOutput.write(1);
    setTimeout( function () {
      pinOutput.write(0);
      setTimeout( function () {
        pinOutput.write(1);
        setTimeout( testLowReadPulse, 200, t );
      }, 300 );
    }, 250 );
  }, 1300 );
}

// Test if a low can be read
function testLowReadPulse(t) {
  pinInput.readPulse('low',5000, function (err,pul) {
    t.error(err,'no error when reading pulse');
    t.equal((Math.abs(350-pul)/350) <= marginOfError, true, 'low 350ms pulse');
    console.error('# pulse length', pul, 'with margin of error', marginOfError);
  });
  setTimeout( function () {
    pinOutput.write(0);
    setTimeout( function () {
      pinOutput.write(1);
      setTimeout( function () {
        pinOutput.write(0);
        setTimeout( testHighPulseTimeout, 200, t );
      }, 300 );
    }, 350 );
  }, 1300 );
}

// Test that a high pulse can timeout and test that timeouts can happen when a pulse is sent
function testHighPulseTimeout(t) {
  pinInput.readPulse('high',100, function (err,pul) {
    t.equal(err == null,false,'error when reading pulse');
    t.equal(err.message, 'SCT timed out while attempting to read pulse', 'high pulse timeout with sent pulse');
  });
  setTimeout( function () {
    pinOutput.write(1);
    setTimeout( function () {
      pinOutput.write(0);
      setTimeout( function () {
        pinOutput.write(1);
        setTimeout( testNeopixelThenReadPulse, 200, t );
      }, 300 );
    }, 450 );
  }, 1300 );
}

// Test that readPulse does not interfere with neopixel
function testNeopixelThenReadPulse(t) {
  neopixels.animate(numNeopixels, Buffer.concat(tracer(numNeopixels)), function (err) {
    t.error(err,'no error when animating neopixels');
    testLowPulseTimeout(t);
  });
  pinInput.readPulse('low',5000, function (err,pul) {
    t.equal(err == null,false,'error when reading pulse');
    t.equal(err.message, 'SCT is already in use by Neopixels', 'readPulse tried using sct (in use by neopixels)');
  });
}

// Test that a low pulse can timeout and test that timeouts can happen when a pulse is not sent
function testLowPulseTimeout(t) {
  pinInput.readPulse('low',5000, function (err,pul) {
    t.equal(err == null,false,'error when reading pulse');
    t.equal(err.message, 'SCT timed out while attempting to read pulse', 'low pulse timeout without pulse sent');
    t.end();
  });
  testReadPulseThenReadPulse(t);
}

// Test that readPulse cannot interfere with another readPulse process
function testReadPulseThenReadPulse(t) {
  pinInput.readPulse('high',5000, function (err,pul) {
    t.equal(err == null,false,'error when reading pulse');
    t.equal(err.message, 'SCT is already in use by Read Pulse', 'readPulse tried using sct (in use by readPulse)');
  });
}

// The animation to run on neopixel strip/ring
function tracer(numLEDs) {
  var trail = 5;
  var arr = new Array(numLEDs);
  for (var i = 0; i < numLEDs; i++) {
    var buf = new Buffer(numLEDs * 3);
    buf.fill(0);
    for (var col = 0; col < 3; col++){
      for (var k = 0; k < trail; k++) {
        buf[(3*(i+numLEDs*col/3)+col+1 +3*k)] = 0xFF*(trail-k)/trail;
      }
    }
    arr[i] = buf;
  }
  return arr;
}
