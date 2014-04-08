/*
 * sdram_init.h
 *
 *  Created on: July 27, 2013
 *      Author: jia
 *      based off of the sdram-issi branch
 */


#ifndef SDRAM_INIT_H_
#define SDRAM_INIT_H_

/* SDRAM Address Base for DYCS0*/
#define SDRAM_ADDR_BASE		0x28000000
/* SDRAM test size 32MB*/
#define SDRAM_SIZE			(32*1024*1024)
/* EMC MAX Clock */
#define MAX_EMC_CLK			144000000

#ifdef __cplusplus
extern "C" {
#endif

void SDRAM_Init();
#ifdef __cplusplus
};
#endif

#endif
