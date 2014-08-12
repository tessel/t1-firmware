// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "tm.h"
#include "hw.h"
#include "string.h"
#include "colony.h"
#include "tessel.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// pushes 1 char at a time to be parsed
bool gps_consume(unsigned char c);

long gps_get_time();
long gps_get_date();
bool gps_get_fix();
long gps_get_altitude();
long gps_get_latitude();
long gps_get_longitude();
int gps_get_satellites();
long gps_get_speed();