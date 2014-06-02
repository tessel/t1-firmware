var tessel = require('tessel');

console.log('1..1');
console.log(tessel.port['A'].id == 'A' ? 'ok' : 'not ok');
console.log('#', tessel.port['A'].id);
