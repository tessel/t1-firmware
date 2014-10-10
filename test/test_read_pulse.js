// GPIO bank vars
var tessel = require('tessel');
var gpio = tessel.port['GPIO'];
var pin_output = gpio.pin['G5'];
var pin_input = gpio.pin['G3'];

// Neopixel related vars
var Neopixels = require('neopixels');
var neopixels = new Neopixels();

// Percent diff of what's read and what's expected due to setTimeout inaccuracy
var MARGIN_OF_ERROR = 0.02;

// The various testss that need to run
var tests = [
  { 'pre_len': 2000,  'pulse_len': 300,  'post_len': 200,  'pull': 'high',  'timeout': 5000, 'neo': 'before' },
]

for (var i = 0; i < tests.length; ++i) {
  run_test(tests[i]);
}

function run_test(test) {

  if (tests[i].neo == 'only') {
    neopixels.animate(100, Buffer.concat(tracer(24))); return;
  }

  if (tests[i].neo == 'before') {
    neopixels.animate(100, Buffer.concat(tracer(24)));
  }

  pin_input.readPulse(test.pull, test.timeout, function (err,pulse_len) {
    if (err) {
      console.log('[T] Failed - expected',test.pulse_len.toString()+'ms',
                  test.pull,'pulse and',err.message);
    } else {
      var moe = (Math.abs(test.pulse_len-pulse_len)/test.pulse_len)
      var status = (moe <= MARGIN_OF_ERROR) ? 'Passed' : 'Failed';
      console.log('[T]',status,'- expected',
                  test.pulse_len.toString()+'ms',test.pull,'pulse and read a',
                  pulse_len.toFixed(2).toString()+'ms pulse (',
                  (moe*100).toFixed(2).toString()+'% diff',(status=='Passed'?'<':'>'),
                  (MARGIN_OF_ERROR*100).toFixed(2).toString()+'% margin of error )');
    }
  });

  if (test.pulse_len) {
    send_pulse_out(test.pull, test.pre_len, test.pulse_len, test.post_len);
  }

  if (tests[0].neo == 'after') {
    neopixels.animate(100, Buffer.concat(tracer(24)));
  }

}


// Outputs a pulse to the output pin
function send_pulse_out(pull, pre_len, pulse_len, post_len) {
  var p = (pull == 'high') ? 1 : 0;
  pin_output.pull((pull=='high')?'pulldown':'pullup');
  setTimeout( function () {
    pin_output.write(p);
    setTimeout( function () {
      pin_output.write(!p);
      setTimeout ( function () {
        pin_output.write(p);
      }, post_len );
    }, pulse_len );
  }, pre_len );
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
