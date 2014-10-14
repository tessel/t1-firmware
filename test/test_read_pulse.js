// GPIO bank vars
var tessel = require('tessel');
var gpio = tessel.port['GPIO'];
var pin_output = gpio.pin['G5'];
var pin_input = gpio.pin['G3'];
var test = require('tape');

// Neopixel related vars
var Neopixels = require('neopixels');
var neopixels = new Neopixels();
var numNeopixels = 24;

// Percent diff of what's read and what's expected due to setTimeout inaccuracy
var marginOfError = 0.02;

pin_output.write(0);
var date = new Date();
test('testing readPulse functionality', function (t) {
  // test if a high pulse can be read and that neopixel does not interfere it
  pin_input.readPulse('high',5000, function (err,pul) {
    t.error(err,'no error when reading 250ms pulse');
    t.equal((Math.abs(250-pul)/250) <= marginOfError, true, 'high 250ms pulse');
  });
  // this is neopixel trying to interfere with readPulse
  neopixels.animate(numNeopixels, Buffer.concat(tracer(numNeopixels)), function (err) {
    t.equal(err == null,false,'error when animating neopixel');
    t.equal(err.message, 'SCT is already in use by Read Pulse', 'neopixel tried using sct (in use by readPulse)');
  });
  setTimeout( function () {
    pin_output.write(1);
    setTimeout( function () {
      pin_output.write(0);
      setTimeout( function () {
        pin_output.write(1);
        setTimeout( function () {
          //  test if a low can be read
          pin_input.readPulse('low',5000, function (err,pul) {
            t.error(err,'no error when reading pulse');
            t.equal((Math.abs(350-pul)/350) <= marginOfError, true, 'low 350ms pulse');
          });
          setTimeout( function () {
            pin_output.write(0);
            setTimeout( function () {
              pin_output.write(1);
              setTimeout( function () {
                pin_output.write(0);
                setTimeout( function () {
                  // test that a high pulse can timeout and test that timeouts can happen when a pulse is sent
                  pin_input.readPulse('high',100, function (err,pul) {
                    t.equal(err == null,false,'error when reading pulse');
                    t.equal(err.message, 'SCT timed out while attempting to read pulse', 'high pulse timeout with sent pulse');
                  });
                  setTimeout( function () {
                    pin_output.write(1);
                    setTimeout( function () {
                      pin_output.write(0);
                      setTimeout( function () {
                        pin_output.write(1);
                        setTimeout( function () {
                          // test that readPulse does not interfere with neopixel
                          neopixels.animate(numNeopixels, Buffer.concat(tracer(numNeopixels)), function (err) {
                            t.error(err,'no error when animating neopixels');
                            // test that a low pulse can timeout and test that timeouts can happen when a pulse is not sent
                            pin_input.readPulse('low',5000, function (err,pul) {
                              t.equal(err == null,false,'error when reading pulse');
                              t.equal(err.message, 'SCT timed out while attempting to read pulse', 'low pulse timeout without pulse sent');
                              t.end();
                            });
                            // test that readPulse cannot interfere with another readPulse process
                            pin_input.readPulse('high',5000, function (err,pul) {
                              t.equal(err == null,false,'error when reading pulse');
                              t.equal(err.message, 'SCT is already in use by Read Pulse', 'readPulse tried using sct (in use by readPulse)');
                            });
                          });
                          // this is readPulse trying to interfere with neopixel
                          pin_input.readPulse('low',5000, function (err,pul) {
                            t.equal(err == null,false,'error when reading pulse');
                            t.equal(err.message, 'SCT is already in use by Neopixels', 'readPulse tried using sct (in use by neopixels)');
                          });
                        }, 200 );
                      }, 300 );
                    }, 450 );
                  }, 1300 );
                }, 200 );
              }, 300 );
            }, 350 );
          }, 1300 );
        }, 200 );
      }, 300 );
    }, 250 );
  }, 1300 );
});

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
