var tessel = require('tessel');
var port = tessel.port['A'];
var test = require('tinytap');
var async = require('async');

test.count(2);

var blueToothSerial = new port.UART();

async.series([
  test('can set dataBits', function(t) {
    var dataBitsSetting = 8;
    blueToothSerial.setDataBits(dataBitsSetting);
    t.equal(blueToothSerial.dataBits, tessel.UARTDataBits[dataBitsSetting], 'dataBits not set correctly');
    t.end();
  }),

  test('can set parity', function(t) {
    var paritySetting = "none";
    blueToothSerial.setParity(paritySetting);
    t.equal(blueToothSerial.parity, tessel.UARTParity[paritySetting], 'dataBits not set correctly');
    t.end();
  }),
  ], function(err) {
    console.log('error running tests', err);
  }
);
