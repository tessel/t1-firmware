/*
 SETUP: Connect G1 and G2 on GPIO bank
*/

var tessel = require('tessel'),
    test = require('tape'),
    pin = tessel.port['GPIO'].digital[0].input(),
    trigger = tessel.port['GPIO'].digital[1].output().low();

if (pin.read() != 0) {
  throw new Error("You must connect pins G1 and G2 on the GPIO bank prior to running test");
}

test('port name', function(t) {
  (tessel.port['A'].id == 'A')
  t.end();
})

var invalidLevelError = new Error("You cannot use 'on' with level interrupts. You can only use 'once'.");

test('registering level listeners', function(t) {
  t.doesNotThrow(pin.once.bind(pin, 'high', function(){}, null, 'Does not throw an error calling `on` for high'));
  t.doesNotThrow(pin.removeAllListeners.bind(pin, 'high', function(){}, null, 'Does not throw an error removing all listeners for high'));
  t.end();
})

test('registering edge listeners', function(t) {
  t.doesNotThrow(pin.on.bind(pin, 'fall', function(){}, null, 'Does not throw an error calling `on` for fall'));
  t.doesNotThrow(pin.removeAllListeners.bind(pin, 'fall', function(){}, null, 'Does not throw an error removing all listeners for rise'));
  t.end();
});

test('Using "on" with level triggers', function(t) {
  t.throws(pin.on.bind(pin, 'high', function() {}), invalidLevelError, "Throws an error when using on with 'high'");
  t.end();
});

test('Using "on" with high level emits an error', function(t) {
  pin.once('error', function() {
    t.end();
  })
  t.doesNotThrow(pin.on.bind(pin, 'high', function() {}), invalidLevelError, "Emits an error when using on with 'high'");
});

test('Using "on" with low level triggers with an error', function(t) {
  pin.once('error', function() {
    t.end();
  })
  t.doesNotThrow(pin.on.bind(pin, 'low', function() {}), invalidLevelError, "Emits an error when using on with 'low'");
});

test('Does not throw an error with non-interrupt events', function(t) {
  t.doesNotThrow(pin.once.bind(pin, 'foo', function() {t.end()}), new Error, "Allows non-interrupt events to be used.'");
  pin.emit('foo');
});

test('pin creates and clears interrupt datastructure', function(t) {
  function c() {};
  pin.on('rise', c);
  t.ok(pin.interrupts, 'pin has interrupts');
  t.ok(pin.interrupts['edge'], 'pin has edge interrupt');
  t.equal(pin.interrupts['edge'].mode, 1, 'pin has correct mode');
  pin.removeListener('rise', c);
  t.notOk(pin.interrupts['edge']);
  t.end();
});

test('pin creates different types of interrupts', function(t) {
  function c() {};
  pin.on('rise', c);
  pin.once('high', c);
  t.ok(pin.interrupts, 'pin has interrupts');
  t.equal(Object.keys(pin.interrupts).length, 2, 'there are two different interrupts for this pin');
  t.ok(pin.interrupts['edge'], 'pin has edge interrupt');
  t.ok(pin.interrupts['level'], 'pin has level interrupt');
  t.equal(pin.interrupts['level'].mode, 1 << 2, 'pin has correct mode');
  pin.removeListener('rise', c);
  pin.removeAllListeners('high');
  t.notOk(pin.interrupts['edge'], 'there are no more edge interrupts');
  t.notOk(pin.interrupts['level'], 'there are no more level interrupts');
  t.end();
});


test('real simple rise test', function(t) {
  trigger.low();
  pin.once('rise', function(time, mode) {
    console.log('hit~', time, mode);
    trigger.low();
    t.end();
  });
  trigger.high();
})

test('real simple fall test', function(t) {
  trigger.high();
  pin.once('fall', function(time, mode) {
    console.log('fall hit~', time, mode);
    t.end();
  });
  trigger.low();
})

test('real simple low test', function(t) {
  trigger.low();
  pin.once('low', function(time, mode) {
    console.log('low hit~', time, mode);
    t.end();
  });
})

test('real simple high test', function(t) {
  trigger.high();
  pin.once('high', function(time, mode) {
    console.log('high hit~', time, mode);
    t.end();
  });
})

test('remove all listeners with no specified mode', function(t) {
  pin.removeAllListeners();
  t.end();
})

test('removing change and have rise still work', function(t) {
  trigger.low();

  pin.on('rise', function(time) {
    console.log('got the time', time);
    t.equal(pin.interrupts['edge'].mode, 1, "Both change and rise interrupts are set.");
    pin.removeAllListeners();
    t.equal(Object.keys(pin.interrupts).length, 0, "There are no more listeners");
    t.end();
  });

  pin.once('change', function(time, trig) {
    console.log('got the change...')
  });
  t.equal(pin.interrupts['edge'].mode, 3, "Both change and rise interrupts are set.");
  trigger.high();
})

test('removing change and have rise and fall still work', function(t) {
  trigger.high();

  pin.on('rise', function(time) {
    console.log('got the time', time);
    t.equal(pin.interrupts['edge'].mode, 3, "Both fall and rise interrupts are set.");
    t.equal(Object.keys(pin.interrupts).length, 1, "There is one more listener");
    trigger.toggle();
  });

  pin.on('fall', function(time) {
    console.log("Fallign man!");
    t.equal(pin.interrupts['edge'].mode, 3, "Both fall and rise interrupts are set.");
    pin.removeAllListeners();
    t.end();
  });

  pin.once('change', function(time, trig) {
    t.equal(pin.interrupts['edge'].mode, 3, "All interrupts are set.");
  });
  t.equal(pin.interrupts['edge'].mode, 3, "Both change and rise interrupts are set.");
  trigger.toggle();
});

test('live event triggers with both modes', function(t) {
  trigger.high();
  var hit = 0;
  pin.once('fall', function(time) {
    if (hit >= 1) {
      t.end();
    }
    else {
      hit++;
    }
  });

  pin.once('low', function(time) {
    if (hit >=1 ) {
      t.end();
    }
    else {
      hit++;
    }
  });

  t.equal(Object.keys(pin.interrupts).length, 2, "there are two separate interrupts");
  trigger.low();
});

test('a bunch of repeated rises', function(t) {
  var interval;
  var times = 0;

  pin.on('rise', function riseListener() {
    times++;
    console.log('rose ', times, ' times');
    if (times > 5) {
      clearInterval(interval);
      pin.removeListener('rise', riseListener);
      t.end();
    }
  });

  interval = setInterval(function() {
    trigger.toggle();
  }, 200);
})


test('a bunch of repeated falls', function(t) {
  var interval;
  var times = 0;

  pin.on('fall', function fallListener() {
    times++;
    console.log('fell ', times, ' times');
    if (times > 5) {
      clearInterval(interval);
      pin.removeListener('fall', fallListener);
      t.end();
    }
  });

  interval = setInterval(function() {
    trigger.toggle();
  }, 200);
})

test('a bunch of repeated levels', function(t) {
  var interval;
  var times = 0;

  pin.once('high', function highListener() {

    pin.once('high', function highListener() {
      clearInterval(interval);
      t.end();
    })

  });

  interval = setInterval(function() {
    trigger.toggle();
  }, 200);
})

test('using too many interrupts, if you can believe it', function(t) {
  pin.once('high', function() {});
  pin.on('rise', function() {});
  var pin1 = tessel.port['C'].digital[0]
  pin1.on('rise', function() {});
  pin1.once('low', function() {});
  var pin2 = tessel.port['C'].digital[1];
  pin2.on('rise', function() {});
  pin2.once('low', function() {});
  var pin3 = tessel.port['C'].digital[2];
  pin3.on('error', function(err){
    t.ok(err, "An error was thrown after using too many interrupts");
    pin.removeAllListeners();
    pin1.removeAllListeners();
    pin2.removeAllListeners();
    pin3.removeAllListeners();
    t.end();
  });
  pin3.on('rise', function() {});
  pin3.once('high', function() {});
});

test('Removing one interrupt of two from the same pin', function(t) {
  trigger.high();
  var interval;
  pin.once('rise', function(time) {
    clearInterval(interval);
    t.end();
  });

  pin.once('fall', function(time) {
  });

  interval = setInterval(function() {
    trigger.toggle();
  }, 100);
});
