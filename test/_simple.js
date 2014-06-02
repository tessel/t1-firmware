var tessel = require('tessel');

console.log('1..1');
var interval = 0;
var id = setInterval(function () {
	var mod = interval % 4
	console.log(mod)
	tessel.led[0].output(mod != 0);
	tessel.led[1].output(mod != 1);
	tessel.led[2].output(mod != 2);
	tessel.led[3].output(mod != 3);
	interval++;
	if (interval > 15) {
		clearInterval(id)
		console.log('ok')
		tessel.led[0].low()
		tessel.led[1].low()
		tessel.led[2].low()
		tessel.led[3].low()
	}
}, 100);