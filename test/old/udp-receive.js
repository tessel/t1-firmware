var dgram = require("dgram");

var server = dgram.createSocket("udp4");

server.on("message", function (msg, rinfo) {
  console.log("server got: " + msg + " from " + rinfo.address + ":" + rinfo.port);
  server.send(msg, 0, msg.length, 4444, rinfo.address, function(err, bytes) {
    console.log('[sent response]');
  });
});

server.on("listening", function () {
  var address = server.address();
  console.log("server listening " + address.address + ":" + address.port);
});

server.bind(4444);
