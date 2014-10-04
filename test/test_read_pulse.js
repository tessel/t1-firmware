/* The pins that are being used as input and output */
var tessel = require('tessel');
var gpio = tessel.port['GPIO'];
var pin_output = gpio.pin['G5'];
var pin_input = gpio.pin['G3'];

/* The amount of time to pause inbetween tests (ms) */
var TIME_OUT = 1000;

/* Configure the input pin to be input and a pulldown */
pin_input.input();
pin_input.pull('pulldown');

/* The various tests that need to run */
var tests = [
    { 'pre_pulse_length': 150, 'pulse_length': 250, 'post_pulse_length': 350, 'pull': 'high', 'timeout': 5000, 'callback': cb_read_pulse },
]

/* Loop though the tests and make sure that they all work */
for (var i = 0; i < tests.length; ++i) {
    run_test(tests[i]);
}

function run_test(test) {
    console.log('TEST:',test.pulse_length,'ms',test.pull,'pulse with a',test.timeout,'ms timeout');
    console.log('--------------------------------------------------------------------------------');
    pull_output(test.pull);
    pin_input.readPulse(test.pull, test.timeout, function (err,pulsetime) {
        if (err) { console.log("TIMED OUT"); }
        else if ( Math.abs(pulsetime - test.pulse_length) < 1 ) { console.log('GOOD READ'); }
        else { console.log('BAD READ:',pulsetime,'!=',test.pulse_length); }
    });
    send_pulse_out(test.pull, test.pre_pulse_length, test.pulse_length, test.post_pulse_length);
}

/* Pulls the output pin 'pulldown' or 'pullhigh' inverse of the actual pull */
function pull_output(pull) {
    var pulli = 'pulldown';
    if (pull == 'low') { pulli = 'pullup'; }
    pin_output.pull(pulli);
}

/* Outputs a pulse to the output pin */
function send_pulse_out(pull,pre_pulse_length,pulse_length,post_pulse_length) {
    var ipi = 0;
    if (pull == 'high') { ipi = 1; } 
    write_output(ipi,pre_pulse_length);
    write_output(!ipi, pre_pulse_length+pulse_length);
    write_output(ipi, pre_pulse_length+pulse_length+post_pulse_length);
}

/* Writes the value to output on the pin */
function write_output(pull,time) {
    setTimeout(function () {
        pin_output.write(pull);
    }, time);
}
