#include "gps-nema.h"

const char *gps_stream = 
	"$GPGSA,A,3,22,18,21,27,14,19,,,,,,,3.5,1.4,3.2*32\r\n"
	"$GPRMC,043621.000,A,3752.0471,N,12217.4709,W,0.32,354.03,120814,,,A*7C\r\n"
	"$GPGGA,043622.000,3752.0470,N,12217.4709,W,1,06,1.4,20.2,M,-25.0,M,,0000*5B\r\n"
	"$GPGSA,A,3,22,18,21,27,14,19,,,,,,,3.5,1.4,3.2*32\r\n"
	"$GPRMC,043622.000,A,3752.0470,N,12217.4709,W,0.25,354.03,120814,,,A*78\r\n"
	"$GPGGA,043623.000,3752.0468,N,12217.4711,W,1,05,2.2,20.1,M,-25.0,M,,0000*5F\r\n"
	"$GPGSA,A,3,22,18,21,27,14,,,,,,,,9.1,2.2,8.9*31\r\n"
	"$GPGSV,3,1,10,22,68,295,19,18,58,042,25,21,52,102,21,27,51,283,15*7F\r\n"
	"$GPGSV,3,2,10,14,42,180,30,19,28,312,04,24,14,084,15,57,20,108,*74\r\n"
	"$GPGSV,3,3,10,15,15,043,,16,07,244,*7D\r\n"
	"$GPRMC,043623.000,A,3752.0468,N,12217.4711,W,0.11,354.03,120814,,,A*7E\r\n"
	"$GPGGA,043624.000,3752.0468,N,12217.4712,W,1,05,2.2,20.1,M,-25.0,M,,0000*5B\r\n"
	;


void gps_nema_test(){
	while (*gps_stream){
		if (gps_consume(*gps_stream++)) {
			// print out the details
			TM_DEBUG("gps_get_time", gps_get_time());
			TM_DEBUG("gps_get_date", gps_get_date());
			TM_DEBUG("gps_get_fix", gps_get_fix());
			TM_DEBUG("gps_get_altitude", gps_get_altitude());
			TM_DEBUG("gps_get_latitude", gps_get_latitude());
			TM_DEBUG("gps_get_longitude", gps_get_longitude());
			TM_DEBUG("gps_get_satellites", gps_get_satellites());
			TM_DEBUG("gps_get_speed", gps_get_speed());
		} else {
			TM_DEBUG("nope");
		}
	}
}

