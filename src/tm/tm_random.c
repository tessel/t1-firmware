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

#define RND_BYTES  16



static time_t seed_time = 0;
static time_t check_time = 0;

unsigned px_acquire_system_randomness(uint8_t *buf)
{
	uint32_t uptime = tm_uptime_micro();
	uint32_t analog1 = hw_analog_read(ADC_5);
	uint32_t analog2 = hw_analog_read(ADC_7);
	uint32_t analog3 = hw_analog_read(E_A1);

	memcpy(&buf[0], (void*) &uptime, sizeof(uint32_t));
	memcpy(&buf[4], (void*) &analog1, sizeof(uint32_t));
	memcpy(&buf[8], (void*) &analog2, sizeof(uint32_t));
	memcpy(&buf[12], (void*) &analog3, sizeof(uint32_t));

	return RND_BYTES;
}

int tm_entropy_seed ()
{
	uint8_t		buf[1024];
	int			n;
	time_t		t;
	int			skip = 1;
	size_t      read;
	int         status;

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