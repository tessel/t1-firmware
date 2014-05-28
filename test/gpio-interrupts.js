// NOTE: bridge MOSI and MISO pins with a jumper
var port = require('tessel').port['B'],
    spi = new port.SPI(),
    tx = new Buffer(64);
for (var i = 0, len = tx.length; i < len; ++i) {
    tx[i] = i;
}

spi.on('ready', function () {
    //spi.transfer(tx, function (e,rx) {
    spi.send(tx, function (e,rx) {
        console.log("TX:", tx);
        console.log("RX:", rx);

        // try again…
        spi.transfer(tx, function (e,rx2) {
            console.log("RX:", rx2);
        });
    });
});