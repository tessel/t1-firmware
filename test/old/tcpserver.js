var http = require('http');
http.createServer(function (req, res) {
  res.writeHead(200, {'Content-Type': 'text/plain', 'Content-Length': 'Hello World\n'.length});
  res.end('Hello World\n');
}).listen(4444, '10.1.90.138');
console.log('Server running at http://10.1.90.138:4444/');