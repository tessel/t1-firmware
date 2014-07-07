/*
 SETUP: Connect MOSI and MISO on any port
*/

var tessel = require('tessel'),
    test = require('tape'),
    hardware = tessel.port['A'],
    chipSelect = hardware.digital[0],
    spi = hardware.SPI({chipSelect:chipSelect});

// function tap (max) { tap.t = 1; console.log(tap.t + '..' + max); };
// function ok (a, d) { console.log(a ? 'ok ' + (tap.t++) + ' -' : 'not ok ' + (tap.t++) + ' -', d); }

Buffer.prototype.string = function () {
  return this.toString('hex');
}

var buffer = new Buffer([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]);

test('transfering a chunk without batching works', function(t) {
  spi.transferBatch(buffer, {chunking:buffer.length}, function (err, rx) {
    t.equal(rx.string(), buffer.string());
    t.end();
  });
})

test('transfering chunks with batching and no remainder works', function(t) {
  spi.transferBatch(buffer, {chunking:3}, function (err, rx) {
    t.equal(buffer.string(), rx.string());
    t.end();
  });
})

test('transfering chunks with a remainder but no batching works', function(t) {
  spi.transferBatch(buffer, {chunking:buffer.length - 1}, function (err, rx) {
    t.equal(rx.string(), buffer.string());
    t.end();
  });
})

test('transfering chunks with a remainder and batching works', function(t) {
  spi.transferBatch(buffer, {chunking:Math.floor(buffer.length/2)}, function (err, rx) {
    t.equal(buffer.string(), rx.string());
    t.end();
  });
})

test('transfering chunks with a start offset works', function(t) {
  spi.transferBatch(buffer, {start:3, chunking:buffer.length}, function (err, rx) {
    t.equal(rx.string(), buffer.slice(0,buffer.length - 3).string());
    t.end();
  });
})

test('repeatedly transfering chunks works', function(t) {
  spi.transferBatch(buffer, {chunking: 2, repeat:4}, function (err, rx) {
    t.equal(buffer.string(), rx.string());
    t.end();
  });
})
