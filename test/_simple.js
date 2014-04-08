var tessel = require('tessel');

console.log('1..1');
var interval = 0;
var id = setInterval(function () {
	var mod = interval % 4
	console.log(mod)
	tessel.led(1).set(mod != 0);
	tessel.led(2).set(mod != 1);
	tessel.led(4).set(mod != 2);
	tessel.led(3).set(mod != 3);
	interval++;
	if (interval > 15) {
		clearInterval(id)
		console.log('ok')
		tessel.led(1).low()
		tessel.led(2).low()
		tessel.led(4).low()
		tessel.led(3).low()
	}
}, 100);