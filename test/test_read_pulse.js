var tessel = require('tessel');
var gpio = tessel.port['GPIO'];
var pin_output = gpio.pin['G4'];
var pin_input = gpio.pin['G3'];

// Percent diff of what's read and what's expected. setTimeout inaccuracy
var MARGIN_OF_ERROR = 0.02;

// The various tests that need to run
var tests = [
  { 'pre_len': 200,  'pulse_len': 300,  'post_len': 200,  'pull': 'high',  'timeout': 5000 },
]

// Loop though the tests and make sure that they all work
for (var i = 0; i < tests.length; ++i) {
  run_test(tests[i])
}

// Run the test and output results
function run_test(test) {
  pull_output(test.pull);
  pin_input.readPulse(test.pull, test.timeout, function (err,pulse_len) {
    if (err) {
      console.log('[T] Failed - expected',test.pulse_len.toString()+'ms',
                  test.pull,'pulse and timed out');
      return;
    }
    var moe = get_margin_of_error(test.pulse_len,pulse_len);
    var status = (moe <= MARGIN_OF_ERROR) ? 'Passed' : 'Failed';
    console.log('[T]',status,'- expected',
                test.pulse_len.toString()+'ms',test.pull,'pulse and read a',
                pulse_len.toFixed(2).toString()+'ms pulse (',
                (moe*100).toFixed(2).toString()+'% margin of error )');
  });
  send_pulse_out(test.pull, test.pre_len, test.pulse_len, test.post_len);
}

// Pulls the output pin 'pulldown' or 'pullhigh' inverse of the actual pull
function pull_output(pull) {
  pin_output.pull((pull == 'low')?'pullup':'pulldown');
}

// Outputs a pulse to the output pin
function send_pulse_out(pull,pre_len,pulse_len,post_len) {
  var pull_i = (pull == 'high') ? 1 : 0;
  write_output(pull_i,pre_len);
  write_output(!pull_i, pre_len+pulse_len);
  if (pull_i) { write_output(pull_i, pre_len+pulse_len+post_len); }
}

// Writes the value to output on the pin
function write_output(pull,time) {
  setTimeout(function () {
    pin_output.write(pull);
  }, time);
}

// Returns the margin of error
function get_margin_of_error(exp,act) {
  return (Math.abs(exp-act)/exp);
}
