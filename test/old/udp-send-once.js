var dgram = require("dgram");

var server = dgram.createSocket("udp4");

if (process.argv.length < 4) {
  console.error('Usage: node udpsend ip port');
  process.exit(1);
}

var msg = new Buffer('Message to ' + process.argv.slice(2).join(' '));

server.bind(4444, function () {
  server.send(msg, 0, msg.length, process.argv[3], process.argv[2], function(err, bytes) {
    console.log('SENT', err, bytes, Date.now());
    process.exit(0);
  });
});