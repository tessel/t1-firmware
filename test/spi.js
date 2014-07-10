var tessel = require('tessel');
var test = require('tape');

var hardware = tessel.port['A'];
var spi = hardware.SPI({chipSelect:hardware.digital[1]});
var failed = false;

spi.initialize();

function testBuf(len) {
  var buf = new Buffer(len);
  for (var i = 0; i<buf.length; i++) {
    buf[i] = i;
  }
  return buf;
}

function wrapCallback(t, cb) {
  var timeout = setTimeout(function fail() {
    t.fail("test timed out");
  }, 10000);
  return function() {
    clearTimeout(timeout);
    cb.apply(null, arguments);
  };
}

// test('small async transfer', function(t) {
//   var outBuf = testBuf(13);
//   spi.transfer(outBuf, wrapCallback(t, function(err, d) {
//     t.error(err);
//     t.equal(d.toString('hex'), outBuf.toString('hex'));
//     t.end();
//   }));
// });

test('batch transfer with chunks', function(t) {
  var outBuf = testBuf(12);
  spi.transferBatch(outBuf, {chunkSize:3}, wrapCallback(t, function(err, d) {
    t.error(err);
    t.equal(d.toString('hex'), outBuf.toString('hex'));
    t.end();
  }));
})

// test('batch transfer with chunks and repeating', function(t) {
//   var outBuf = testBuf(12);
//   spi.transferBatch(outBuf, {chunkSize:2, repeat:2}, wrapCallback(t, function(err, d) {
//     t.error(err);
//     t.equal(d.toString('hex'), outBuf.toString('hex'));
//     t.end();
//   }));
// })

// test('batch transfer with no options', function(t) {
//   var outBuf = testBuf(12);
//   spi.transferBatch(outBuf, wrapCallback(t, function(err, d) {
//     t.error(err);
//     t.equal(d.toString('hex'), outBuf.toString('hex'));
//     t.end();
//   }));
// })

// test('large async transfer', function(t) {
//   var outBuf = testBuf(9000);
//   spi.transfer(outBuf, wrapCallback(t, function(err, d) {
//     t.error(err);
//     t.equal(d.toString('hex'), outBuf.toString('hex'));
//     t.end();
//   }));
// });

