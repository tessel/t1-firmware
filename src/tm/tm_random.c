// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "tm.h"
#include "hw.h"
#include "tessel.h"
#include "otprom.h"

// seed entropy

/*
 * System reseeds should be separated at least this much.
 */
#define SYSTEM_RESEED_MIN			(20*60)		/* 20 min */
/*
 * How often to roll dice.
 */
#define SYSTEM_RESEED_CHECK_TIME	(10*60)		/* 10 min */
/*
 * The chance is x/256 that the reseed happens.
 */
#define SYSTEM_RESEED_CHANCE		(4) /* 256/4 * 10min ~ 10h */

/*
 * If this much time has passed, force reseed.
 */
#define SYSTEM_RESEED_MAX			(12*60*60)	/* 12h */

#define RND_BYTES 1024



static time_t seed_time = 0;
static time_t check_time = 0;

unsigned px_acquire_system_randomness(uint8_t *buf)
{

	for (int i=0; i<RND_BYTES; i+=4) {
		buf[i]   = (uint8_t) tm_uptime_micro();
		buf[i+1] = (uint8_t) hw_analog_read(ADC_5);
		buf[i+2] = (uint8_t) hw_analog_read(ADC_7);
		buf[i+3] = (uint8_t) hw_analog_read(E_A1);
	}

	return RND_BYTES;
}

int tm_entropy_seed ()
{
	uint8_t		buf[RND_BYTES];
	int			n;
	time_t		t;
	int			skip = 1;
	size_t      read;
	int         status = 0;

	t = time(NULL);

	if (seed_time == 0)
		skip = 0;
	else if ((t - seed_time) < SYSTEM_RESEED_MIN)
		skip = 1;
	else if ((t - seed_time) > SYSTEM_RESEED_MAX)
		skip = 0;
	else if (check_time == 0 ||
			 (t - check_time) > SYSTEM_RESEED_CHECK_TIME)
	{
		check_time = t;

		/* roll dice */
		tm_random_bytes((uint8_t*) buf, 1, &read);
		skip = buf[0] >= SYSTEM_RESEED_CHANCE;
	}
	/* clear 1 byte */
	memset(buf, 0, sizeof(buf));

	if (skip)
		return 0;

	n = px_acquire_system_randomness(buf);
	if (n > 0)
		status = tm_entropy_add((const uint8_t*) buf, n);

	seed_time = t;
	memset(buf, 0, sizeof(buf));
	return status;
}