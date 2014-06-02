// Copyright 2014 Technical Machine, Inc. See the COPYRIGHT
// file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

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
