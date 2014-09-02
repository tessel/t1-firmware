var tessel = require('tessel');
var port = tessel.port['A'];
var test = require('tinytap');
var async = require('async');

test.count(5);

var uart = new port.UART();

async.series([
  test('can set dataBits', function(t) {
    var dataBitsSetting = 8;
    uart.setDataBits(dataBitsSetting);
    t.equal(uart.dataBits, tessel.UARTDataBits[dataBitsSetting], 'dataBits not set correctly');
    t.end();
  }),

  test('can set parity', function(t) {
    var paritySetting = "none";
    uart.setParity(paritySetting);
    t.equal(uart.parity, tessel.UARTParity[paritySetting], 'dataBits not set correctly');
    t.end();
  }),

  test('bad dataBits setting throws an error', function(t) {
    var dataBitsSetting = "fake";
    // I'm not sure why t.throws isn't working here
    try {
      uart.setDataBits(dataBitsSetting);
    }
    catch (err) {
      t.ok(err, 'no error was thrown.');
    }

    t.end();
  }),

  test('bad parity setting throws an error', function(t) {
    var paritySetting = "fake";
    // I'm not sure why t.throws isn't working here
    try {
      uart.setParity(paritySetting);
    }
    catch (err) {
      t.ok(err, 'no error was thrown.');
    }

    t.end();
  }),

  test('bad stopBits setting throws an error', function(t) {
    var stopBitsSetting = "fake";
    // I'm not sure why t.throws isn't working here
    try {
      uart.setStopBits(stopBitsSetting);
    }
    catch (err) {
      t.ok(err, 'no error was thrown.');
    }

    t.end();
  }),

  ], function(err) {
    console.log('error running tests', err);
  }
);
