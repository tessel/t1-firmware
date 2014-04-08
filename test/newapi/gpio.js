var tessel = require('tessel');

var gpio = tessel.ports['GPIO'];

// load tessel port
setTimeout(function next () {
	console.log(' sync',
	  'G3 =', gpio.digital[3].readSync(),
	  'A3 =', gpio.analog[3].readSync().toFixed(2));

	gpio.digital[3].read(function (err, d) {
		gpio.analog[3].read(function (err, a) {
			console.log('async',
			  'G3 =', d,
			  'A3 =', a.toFixed(2));

			setTimeout(next, 100);
		})
	})

}, 100);