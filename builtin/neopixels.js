var hw = process.binding('hw');
var util = require('util');
var events = require('events');
var tessel = require('tessel');
/*
 One function to send an animation to Tessel.
 Data gets sent on the GPIO bank's G4
*/

function Neopixels() {
  this.animate = function(numPixels, animationData, pin, callback) {
    var sctStatus = hw.neopixel_animation_buffer(numPixels * 3, animationData, pin.pin);
    if (sctStatus) {
      callback && callback(new Error("SCT is already in use by "+['Inactive','PWM','Read Pulse','Neopixels'][sctStatus]));
    } else {
      // When we finish sending the animation
      process.once('neopixel_animation_complete', function animationComplete(finishedAnimationsMask) {
        var finished;
        if ((finishedAnimationsMask && 2) &
          (pin == tessel.port['GPIO'].digital[3])) {
          finished = tessel.port['GPIO'].digital[3];
        }
        if ((finishedAnimationsMask && 1) &
          (pin == tessel.port['GPIO'].digital[4])) {
          finished = tessel.port['GPIO'].digital[4];
        }
        // Emit an end event
        this.emit('end', pin);
        // If there is a callback, call it
        callback && callback();

      }.bind(this));
    }
  }
}

util.inherits(Neopixels, events.EventEmitter);

module.exports = Neopixels;