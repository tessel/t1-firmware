// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

/* $GPRMC
 * eg1. $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
 * 1    = UTC of position fix
 * 2    = Data status (V=navigation receiver warning)
 * 3    = Latitude of fix
 * 4    = N or S
 * 5    = Longitude of fix
 * 6    = E or W
 * 7    = Speed over ground in knots
 * 8    = Track made good in degrees True
 * 9    = UT date
 * 10   = Magnetic variation degrees (Easterly var. subtracts from true course)
 * 11   = E or W
 * 12   = Checksum
*/


/* $GPGGA
 * eg3. GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
 * 1    = UTC of Position
 * 2    = Latitude
 * 3    = N or S
 * 4    = Longitude
 * 5    = E or W
 * 6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
 * 7    = Number of satellites in use [not those in view]
 * 8    = Horizontal dilution of position
 * 9    = Antenna altitude above/below mean sea level (geoid)
 * 10   = Meters  (Antenna height unit)
 * 11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and
 *        mean sea level.  -=geoid is below WGS-84 ellipsoid)
 * 12   = Meters  (Units of geoidal separation)
 * 13   = Age in seconds since last update from diff. reference station
 * 14   = Diff. reference station ID#
 * 15   = Checksum
 */ 

#include "gps-nmea.h"

#define GPRMC "GPRMC"
#define GPGGA "GPGGA"
#define GPS_BUFF_LEN 30

char gps_buff[GPS_BUFF_LEN] = {'\0'};

unsigned int gps_buff_pos = 0;
int gps_checksum = 0;
bool gps_is_on_checksum = false;
bool gps_ignoring = false;
unsigned int gps_msg_pos = 0;


// static funcs
static bool gps_parse(void);
static void gps_cleanup(void);

enum gps_msg_types {
	GPRMC_MSG = 1,
	GPGGA_MSG = 2,
	IGNORE_MSG = 3
} gps_msg_types;

typedef struct GPSData {
	bool has_fix;
	bool checksum_passed;
	double latitude;
	double longitude;
	double speed;
	double date;
	double timeUTC;
	int satellites;
	double altitude;
} GPSData;

GPSData gps_data = { .has_fix = false, .checksum_passed = false,
	.latitude = 0, .longitude = 0, .speed = 0, .date = 0, .timeUTC = 0,
	.satellites = 0, .altitude = 0};

GPSData gps_data_hold = { .has_fix = false, .checksum_passed = false,
	.latitude = 0, .longitude = 0, .speed = 0, .date = 0, .timeUTC = 0,
	.satellites = 0, .altitude = 0};

bool gps_checksum_ret = false;
// consume a gps character
// returns true if we are ready to parse
// returns false otherwise
bool gps_consume(unsigned char c) {
	if (gps_ignoring && c != '$') {
		return false;
	} 
	switch (c) {
		case ',':
			// nothin is there
			gps_checksum ^= c;
		case '\r':
		case '\n':
			// end of string
		case '*':
			// checksum
			gps_checksum_ret = gps_parse();
			if (c != ',') {
				gps_is_on_checksum = true;
			}
			return gps_checksum_ret;

		case '$':
			// beginning of string
			// clear out the old buffer
			gps_parse();
			gps_cleanup();
			gps_checksum = 0;
			gps_is_on_checksum = false;
			gps_ignoring = false;
			gps_msg_types = IGNORE_MSG;
			gps_msg_pos = 0;
			return false;
		default:
			// all other characters
			gps_buff[gps_buff_pos++] = c;
			if (!gps_is_on_checksum) {
				gps_checksum ^= c;
			}
			return false;

	}

	return false;
}

#define GPS_UNIQUE_CASE(type, pos) (((unsigned)(type) << 5) | pos)
static bool gps_parse(void){
	// if the checksum worked out, set all the values
	if (gps_is_on_checksum) {
		if (gps_checksum == (int)strtol(gps_buff, NULL, 16)) {
			// checksum passed
			gps_data.checksum_passed = true;
			// migrate all data to gps_data_hold
			memcpy(&gps_data_hold, &gps_data, sizeof(GPSData));
			return true;
		} else {
			// checksum failed, discard message and clean up gps
			gps_cleanup();
		}
	}
	// parses the string stored in the buffer
	if (gps_buff_pos == 5 && gps_msg_pos == 0) {
		// if its the first 4 characters (+1 for "$" char), check for nmea string types
		if (!strcmp(GPRMC, gps_buff)) {
			// GPRMC
			gps_msg_types = GPRMC_MSG;
		} else if (!strcmp(GPGGA, gps_buff)) {
			// GPGGA
			gps_msg_types = GPGGA_MSG;
		} else {
			// something else, we should just ignore for now
			gps_ignoring = true;
			gps_msg_types = IGNORE_MSG;
		}
		gps_msg_pos++;
	} else {
		if (gps_msg_types == GPRMC_MSG || gps_msg_types == GPGGA_MSG) {
			switch (GPS_UNIQUE_CASE(gps_msg_types, gps_msg_pos)) {
				case GPS_UNIQUE_CASE(GPRMC_MSG, 1):
				case GPS_UNIQUE_CASE(GPGGA_MSG, 1):
					// UTC of position fix
					gps_data.timeUTC = atof(gps_buff);
					break;
				case GPS_UNIQUE_CASE(GPRMC_MSG, 2):
					// Data status (V=navigation receiver warning)
					gps_data.has_fix = gps_buff[0] == 'A';
					break;
				case GPS_UNIQUE_CASE(GPRMC_MSG, 3):
				case GPS_UNIQUE_CASE(GPGGA_MSG, 2):
					// Latitude
					gps_data.latitude = atof(gps_buff);
					break;
				case GPS_UNIQUE_CASE(GPRMC_MSG, 4):
				case GPS_UNIQUE_CASE(GPGGA_MSG, 3):
					// N or S
					gps_data.latitude =  gps_buff[0] == 'N' ? gps_data.latitude : -gps_data.latitude;
					break;
				case GPS_UNIQUE_CASE(GPRMC_MSG, 5):
				case GPS_UNIQUE_CASE(GPGGA_MSG, 4):
					// Longitude
					gps_data.longitude = atof(gps_buff);
					break;

				case GPS_UNIQUE_CASE(GPRMC_MSG, 6):
				case GPS_UNIQUE_CASE(GPGGA_MSG, 5):
					// E or W
					gps_data.longitude =  gps_buff[0] == 'E' ? gps_data.longitude : -gps_data.longitude;
					break;

				case GPS_UNIQUE_CASE(GPRMC_MSG, 7):
					// Speed over ground in knots
					gps_data.speed = atof(gps_buff);
					break;

				case GPS_UNIQUE_CASE(GPRMC_MSG, 9):
					// UT date
					gps_data.date = atof(gps_buff);
					break;

				case GPS_UNIQUE_CASE(GPGGA_MSG, 6):
					// GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
					gps_data.has_fix = gps_buff[0] > '0';
					break;
					
				case GPS_UNIQUE_CASE(GPGGA_MSG, 7):
					// Number of satellites in use [not those in view]
					gps_data.satellites = atoi(gps_buff);
					break;

				case GPS_UNIQUE_CASE(GPGGA_MSG, 9):
					// Meters  (Antenna height unit)
					gps_data.altitude = atof(gps_buff);
					break;

			}
			gps_msg_pos++;
		}
	}

	// after this has been parsed, reset buffer
	memset(gps_buff, '\0', gps_buff_pos);
	gps_buff_pos = 0;

	return false;
}

static void gps_cleanup(){
	gps_is_on_checksum = false;
	memset(gps_buff, '\0', gps_buff_pos);
	gps_buff_pos = 0;
	// scrub gps data
	gps_data.has_fix = false;
	gps_data.checksum_passed = false;
	gps_data.latitude = 0;
	gps_data.longitude = 0;
	gps_data.speed = 0;
	gps_data.date = 0;
	gps_data.timeUTC = 0;
	gps_data.satellites = 0;
	gps_data.altitude = 0;
}

double gps_nmea_to_deg(double nmea){
	bool negative = false;
	// flip negativity
	if (nmea < 0) {
		negative = true;
		nmea = -nmea;
	}
	// weird nmea gps format
	// find 2 least significant digits after decimal
	double minutes = fmod(nmea, 100);

	// cut it
	// all prev digits + cut portion/60
	return ((floor(nmea/100) ) + minutes/60) * (negative ? -1 : 1);
}

double gps_get_time(){
	if (gps_data_hold.checksum_passed) {
		return gps_data_hold.timeUTC;
	}
	return 0;
}

double gps_get_date(){
	if (gps_data_hold.checksum_passed) {
		return gps_data_hold.date;
	}
	return 0;
}

bool gps_get_fix() {
	if (gps_data_hold.checksum_passed) {
		return gps_data_hold.has_fix;
	}
	return false;
}

double gps_get_altitude() {
	// returns altitude
	if (gps_data_hold.checksum_passed && gps_data_hold.has_fix) {
		return gps_data_hold.altitude;
	}
	return false;
}

double gps_get_latitude() {
	// returns latitude
	if (gps_data_hold.checksum_passed && gps_data_hold.has_fix) {
		return gps_nmea_to_deg(gps_data_hold.latitude);
	}
	return 0;
}

double gps_get_longitude(){
	// returns longitude
	if (gps_data_hold.checksum_passed && gps_data_hold.has_fix) {
		return gps_nmea_to_deg(gps_data_hold.longitude);
	}
	return 0;
}

int gps_get_satellites(){
	// returns num of satellites
	if (gps_data_hold.checksum_passed && gps_data_hold.has_fix) {
		return gps_data_hold.satellites;
	}
	return false;
}

double gps_get_speed(){
	// returns speed
	if (gps_data_hold.checksum_passed && gps_data_hold.has_fix) {
		return gps_data_hold.speed;
	}
	return false;
}
