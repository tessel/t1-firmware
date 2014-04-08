#!/usr/bin/env node
// Usage: quiet "Display String" command command_args

var util = require('util')

var display = process.argv[2];
var command = process.argv[3];
var args = process.argv.slice(4);

function color(color, text) {
  var ansi = util.inspect.colors[color];
  console.log('\x1B['+ansi[0]+'m' + text + '\x1B['+ansi[1]+'m');
}

require('child_process').execFile(command, args, {}, function (error, stdout, stderr) {
  if (error !== null) {
    color('red', display);
    console.log(command + ' ' + args.join(' '));
    console.log(stdout + stderr);
    process.exit(Number(error.code) || 127);
  } else if (stdout || stderr) {
    color('yellow', display);
    console.log(stdout + stderr);
  } else {
    color('green', display);
  }
});
