var wifi = require('wifi-cc3000');
var network = process.argv[2];
var pass = process.argv[3];

wifi.on('connect', function(err, data){
  console.log("connect emitted", err, data);
});

wifi.on('disconnect', function(err, data){
  console.log("disconnect emitted", err, data);
})

wifi.on('timeout', function(err){
  console.log("timeout emitted", err);
});

wifi.on('error', function(err){
  console.log("error emitted", err);
});

process.ref();