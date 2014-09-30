var http = require('http');
var wifi = require('wifi-cc3000');
var network = process.argv[2];
var pass = process.argv[3];

console.log('Loggin in with network:', network, 'password:', pass);

function getMacAddress() {
  return wifi.macAddress();
}

function tryConnect(){
  if (!wifi.isBusy()) {
    console.log("not busy");
    connect();
  } else {
    console.log("is busy, trying again");
    setTimeout(function(){
      tryConnect();
    }, 1000);
  } 
}

function tryDisconnect(){
  wifi.disconnect(function(){
    console.log("is connected", wifi.isConnected() == false); // should be false

    console.log("disable wifi");
    wifi.disable();
    console.log("is connected", wifi.isConnected() == false); // should be false
    console.log("connection data", wifi.connection() == null); // should be null
    console.log("connection enabled", wifi.isEnabled() == false); // should be false

    // renable + reconnect
    wifi.enable();
    console.log("connection enabled", wifi.isEnabled() == true); // should be true

    tryConnect();
  });
}

function connect(){
  wifi.connect({
    security: "wpa2"
    , ssid: new Buffer(network)
    , password: pass
    , timeout: 30
  }, function(err, data){
    console.log("connection data", err, wifi.connection()); // should return json data
    console.log("is connected", wifi.isConnected() == true); // should be true
    // disconnect
    if (!err) {
      // console.log("disconnect");
      // tryDisconnect();

      console.log("try http request");
      var statusCode = 200;
      var count = 1;

      setImmediate(function start () {
        console.log('http request #' + (count++))
        http.get("http://httpstat.us/" + statusCode, function (res) {
        // http.get("http://192.168.128.1/" + statusCode, function (res) {
          console.log('# statusCode', res.statusCode)
          
          var bufs = [];
          res.on('data', function (data) {
            bufs.push(new Buffer(data));
            console.log('# received', new Buffer(data).toString());
          })
          res.on('end', function () {
            console.log('done.');
            setImmediate(start);
          })
        }).on('error', function (e) {
          console.log('not ok -', e.message, 'error event')
          setImmediate(start);
        });
      });

    } else {
      tryConnect();
    }
  });
}

// wifi.on('connect', function(err, data){
//   console.log("connect emittied", err, data);
// });

// wifi.on('disconnect', function(err, data){
//   console.log("disconnect emittied", err, data);
// })

wifi.on('timeout', function(err){
  console.log("timeout emitted");
  connect();
});

wifi.on('error', function(err){
  console.log("error emitted", err);
});

console.log("Mac Address: ", getMacAddress());

tryConnect();

process.ref();
