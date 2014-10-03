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
    { 'pre_pulse_length': 500, 'pulse_length': 250, 'pull': 'low', 'timeout': 500, 'callback': cb_read_pulse },
    //{ 'pre_pulse_length': 500, 'pulse_length': 750, 'pull': 'low', 'timeout': 500, 'callback': cb_read_pulse },
]

/* The amount of time that's needed to pause inbetween tests */
var total_pause = 0;

/* Loop though the tests and make sure that they all work */
for (var i = 0; i < tests.length; ++i) {
    run_test(tests[i]);
}

function run_test(test) {
    total_pause += test.timeout*2;
    setTimeout( function () {
        console.log('TEST:',test.pulse_length,'ms',test.pull,'pulse with a',test.timeout,'ms timeout');
        console.log('--------------------------------------------------------------------------------');
        pull_output(test.pull);
        var read_in = pin_input.readPulse(test.pull, test.timeout, function () {
            if ( Math.abs(read_in - test.pulse_length) < 1 ) { console.log('GOOD READ'); }
            else { console.log('BAD READ:',read_in,'!=',test.pulse_length); }
        });
        send_pulse_out(test.pull, test.pre_pulse_length, test.pulse_length);
    }, total_pause );
}

/* Pulls the output pin 'pulldown' or 'pullhigh' inverse of the actual pull */
function pull_output(pull) {
    var pulli = 'pulldown';
    if (pull == 'low') { pulli = 'pullup'; }
    pin_output.pull(pulli);
}

/* Outputs a pulse to the output pin */
function send_pulse_out(pull,pre_pulse_length,pulse_length) {
    var ipi = 0;
    if (pull == 'high') { ipi = 1; } 
    write_output(ipi,pre_pulse_length);
    write_output(!ipi, pre_pulse_length+pulse_length);
}

/* Writes the value to output on the pin */
function write_output(pull,time) {
    setTimeout(function () {
        pin_output.write(pull);
    }, time);
}
