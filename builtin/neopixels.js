var hw = process.binding('hw');
var util = require('util');
var events = require('events');

/*
 One function to send an animation to Tessel.
 Data gets sent on the GPIO bank's G4
*/

function Neopixels() {
  this.animate = function(numPixels, animationData, callback) {
    var sctStatus = hw.neopixel_animation_buffer(numPixels * 3, animationData);
    if (sctStatus) {
      callback && callback(new Error("SCT is already in use by "+['Inactive','PWM','Read Pulse','Neopixels'][sctStatus]));
    } else {
      // When we finish sending the animation
      process.once('neopixel_animation_complete', function animationComplete() {
        // Emit an end event
        this.emit('end');
        // If there is a callback
        if (callback) {
          // Call it
          callback();
        }
      }.bind(this));
    }
  }
}

util.inherits(Neopixels, events.EventEmitter);

module.exports = Neopixels;