var tessel = require('tessel');

var gpio = tessel.port('gpio');

gpio.gpio(4).output().low()
console.log('A:', gpio.gpio(3).read(), ' == low');
gpio.gpio(3).output().low()
gpio.gpio(4).output().low()

var A = gpio.gpio(3).input();
var B = gpio.gpio(4).input();

console.log('B:', B.read(), ' == low')
A.setOutput(1);
console.log('B:', B.read(), ' == high')
B.output().high();
console.log('A:', A.read(), ' == high');

