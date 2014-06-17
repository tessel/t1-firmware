// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <tm.h>

static double timestamp_base = 0;

double tm_timestamp ()
{
    return timestamp_base + tm_uptime_micro();
}

int tm_timestamp_update (double millis)
{
    timestamp_base = millis;
    return 1;
}

// Called from the timer ISR when the timer wraps
void tm_timestamp_wrapped () {
    timestamp_base += 0xFFFFFFFF;
}
