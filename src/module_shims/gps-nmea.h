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
#include <math.h>

// pushes 1 char at a time to be parsed
bool gps_consume(unsigned char c);

double gps_get_time();
double gps_get_date();
bool gps_get_fix();
double gps_get_altitude();
double gps_get_latitude();
double gps_get_longitude();
int gps_get_satellites();
double gps_get_speed();
