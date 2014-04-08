#include "sdram_init.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_libcfg.h"
#include "lpc18xx_timer.h"
#include "tm.h"

#define EMC_B_ENABLE 					(1 << 19)
#define EMC_ENABLE 						(1 << 0)
#define EMC_CE_ENABLE 					(1 << 0)
#define EMC_CS_ENABLE 					(1 << 1)
#define EMC_CLOCK_DELAYED_STRATEGY 		(0 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY 	(1 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY2 	(2 << 0)
#define EMC_COMMAND_DELAYED_STRATEGY3 	(3 << 0)
#define EMC_INIT(i) 					((i) << 7)
#define EMC_NORMAL 						(0)
#define EMC_MODE 						(1)
#define EMC_PRECHARGE_ALL 				(2)
#define EMC_NOP 						(3)

/* SDRAM Address Base for DYCS0*/
#define SDRAM_ADDR_BASE		0x28000000

/* SDRAM refresh time to 16 clock num */
#define EMC_SDRAM_REFRESH(freq,time)  \
  (((uint64_t)((uint64_t)time * freq)/16000000000ull)+1)

/*-------------------------PRIVATE FUNCTIONS------------------------------*/
/*********************************************************************
 * @brief		Calculate EMC Clock from nano second
 * @param[in]	freq - frequency of EMC Clk
 *						frequency is a variable so cannot present this as a macro
 *						because it always be calculated in run-time..
 * @param[in]	time - nano second
 * @return 		None
 **********************************************************************/
uint32_t NS2CLK(uint32_t freq,uint32_t time){
	freq /= 100000;
 return (time*freq/10000);
}

/*********************************************************************
 * @brief		Init the EMC Controller to connect ex SDRAM
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void SDRAM_Init () {
	uint32_t sdram_speed = CGU_GetPCLKFrequency(CGU_PERIPHERAL_EMC);
	if( sdram_speed > MAX_EMC_CLK*2){ // Output clock is divided by 2
		TM_DEBUG("\r\nCore M3 Clk too high, SDRAM operation may wrong! Should set lower!!\r\n");
		while(1){};
	}

	uint32_t pclk, temp;
	uint64_t tmpclk;
	TIM_TIMERCFG_Type TIM_ConfigStruct;

	(void) temp;
	(void) tmpclk;


	/* Set up EMC pin */
	// data lines (0-15)
	scu_pinmux(	1	,	7	,	MD_PLN_FAST	,	3	);//D0
	scu_pinmux(	1	,	8	,	MD_PLN_FAST	,	3	);//D1
	scu_pinmux(	1	,	9	,	MD_PLN_FAST	,	3	);//D2
	scu_pinmux(	1	,	10	,	MD_PLN_FAST	,	3	);//D3
	scu_pinmux(	1	,	11	,	MD_PLN_FAST	,	3	);//D4
	scu_pinmux(	1	,	12	,	MD_PLN_FAST	,	3	);//D5
	scu_pinmux(	1	,	13	,	MD_PLN_FAST	,	3	);//D6
	scu_pinmux(	1	,	14	,	MD_PLN_FAST	,	3	);//D7
	scu_pinmux(	5	,	4	,	MD_PLN_FAST	,	2	);//D8
	scu_pinmux(	5	,	5	,	MD_PLN_FAST	,	2	);//D9
	scu_pinmux(	5	,	6	,	MD_PLN_FAST	,	2	);//D10
	scu_pinmux(	5	,	7	,	MD_PLN_FAST	,	2	);//D11
	scu_pinmux(	5	,	0	,	MD_PLN_FAST	,	2	);//D12
	scu_pinmux(	5	,	1	,	MD_PLN_FAST	,	2	);//D13
	scu_pinmux(	5	,	2	,	MD_PLN_FAST	,	2	);//D14
	scu_pinmux(	5	,	3	,	MD_PLN_FAST	,	2	);//D15

	// address lines (0-12)
	scu_pinmux(	2	,	9	,	MD_PLN_FAST	,	3	);//A0
	scu_pinmux(	2	,	10	,	MD_PLN_FAST	,	3	);//A1
	scu_pinmux(	2	,	11	,	MD_PLN_FAST	,	3	);//A2
	scu_pinmux(	2	,	12	,	MD_PLN_FAST	,	3	);//A3
	scu_pinmux(	2	,	13	,	MD_PLN_FAST	,	3	);//A4
	scu_pinmux(	1	,	0	,	MD_PLN_FAST	,	2	);	//A5
	scu_pinmux(	1	,	1	,	MD_PLN_FAST	,	2	);	//A6
	scu_pinmux(	1	,	2	,	MD_PLN_FAST	,	2	);	//A7
	scu_pinmux(	2	,	8	,	MD_PLN_FAST	,	3	);//A8
	scu_pinmux(	2	,	7	,	MD_PLN_FAST	,	3	);//A9
	scu_pinmux(	2	,	6	,	MD_PLN_FAST	,	2	);//A10
	scu_pinmux(	2	,	2	,	MD_PLN_FAST	,	2	);//A11
	scu_pinmux(	2	,	1	,	MD_PLN_FAST	,	2	);//A12

	// fake address lines. actually bank select
	scu_pinmux(	2	,	0	,	MD_PLN_FAST	,	2	);//A13
	scu_pinmux(	6	,	8	,	MD_PLN_FAST	,	1	);//A14


//	scu_pinmux(	1	,	4	,	MD_PLN_FAST	,	3	);	//BLS0
//	scu_pinmux(	6	,	6	,	MD_PLN_FAST	,	1	);//BLS1

	scu_pinmux(	6	,	9	,	MD_PLN_FAST	,	3	);//DYCS0
	scu_pinmux(	1	,	6	,	MD_PLN_FAST	,	3	);	//WE
	scu_pinmux(	6	,	4	,	MD_PLN_FAST	,	3	);//CAS
	scu_pinmux(	6	,	5	,	MD_PLN_FAST	,	3	);//RAS

	scu_pinmux(	6	,	11	,	MD_PLN_FAST	,	3	);//CKEOUT0

	scu_pinmux(	6	,	12	,	MD_PLN_FAST	,	3	);//DQMOUT0
	scu_pinmux(	6	,	10	,	MD_PLN_FAST	,	3	);//DQMOUT1

	scu_pinmux(	1	,	3	,	MD_PLN_FAST	,	3	);	//OE

//	scu_pinmux(	1	,	5	,	MD_PLN_FAST	,	3	);	//CS0
//	scu_pinmux(	6	,	3	,	MD_PLN_FAST	,	3	);//CS1
//	scu_pinmux(	6	,	7	,	MD_PLN_FAST	,	1	);//A15

//	scu_pinmux(	10	,	4	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	13	,	0	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	2	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	3	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	4	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	5	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	6	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	7	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	8	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	9	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	10	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	12	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	13	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	15	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	13	,	16	,	MD_PLN_FAST	,	2	);
//	scu_pinmux(	14	,	0	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	1	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	2	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	3	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	4	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	5	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	6	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	7	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	8	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	9	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	10	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	11	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	12	,	MD_PLN_FAST	,	3	);
//	scu_pinmux(	14	,	13	,	MD_PLN_FAST	,	3	);

	/* The recommended power-up sequence for SDRAMs:
		1. Simultaneously apply power to VDD and VDDQ.
		2. Assert and hold CKE at a LVTTL logic LOW since all inputs and outputs are LVTTLcompatible.
		3. Provide stable CLOCK signal. Stable clock is defined as a signal cycling within timing
		constraints specified for the clock pin.
		4. Wait at least 100μs prior to issuing any command other than a COMMAND INHIBIT
		or NOP.
		5. Starting at some point during this 100μs period, bring CKE HIGH. Continuing at least
		through the end of this period, 1 or more COMMAND INHIBIT or NOP commands
		must be applied.
		6. Perform a PRECHARGE ALL command.
		7. Wait at least tRP time; during this time NOPs or DESELECT commands must be given.
		All banks will complete their precharge, thereby placing the device in the all banks
		idle state. (tRP = 15ns)
		8. Issue an AUTO REFRESH command.
		9. Wait at least tRFC time, during which only NOPs or COMMAND INHIBIT commands
		are allowed.
		10. Issue an AUTO REFRESH command.
		11. Wait at least tRFC time, during which only NOPs or COMMAND INHIBIT commands
		are allowed.
		12. The SDRAM is now ready for mode register programming. Because the mode register
		will power up in an unknown state, it should be loaded with desired bit values prior to
		applying any operational command. Using the LMR command, program the mode
		register. The mode register is programmed via the MODE REGISTER SET command
		with BA1 = 0, BA0 = 0 and retains the stored information until it is programmed again
		or the device loses power. Not programming the mode register upon initialization will
		result in default settings which may not be desired. Outputs are guaranteed High-Z
		after the LMR command is issued. Outputs should be High-Z already before the LMR
		command is issued.
		13. Wait at least tMRD time, during which only NOP or DESELECT commands are
		allowed.
		At this point the DRAM is ready for any valid command.
	*/
	/* Select EMC clock-out */
	/*
	 * To use 16-bit wide and 32-bit wide SDRAM interfaces, select the EMC_CLK function
	 * and enable the input buffer (EZI = 1) in all four SFSCLKn registers in the SCU.
	 */
	LPC_SCU->SFSCLK_0 = MD_PLN_FAST;
	LPC_SCU->SFSCLK_1 = MD_PLN_FAST;
	LPC_SCU->SFSCLK_2 = MD_PLN_FAST;
	LPC_SCU->SFSCLK_3 = MD_PLN_FAST;

	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;

	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_Init(LPC_TIMER0, TIM_TIMER_MODE, &TIM_ConfigStruct);

	// AN11508 workaround for EMC.1 bug
	LPC_SCU->EMCDELAYCLK = 0x6;
	// enable emc, normal memory map, normal power
	LPC_EMC->CONTROL 	= 0x00000001;

	// little endian
	// 2. Assert and hold CKE at a LVTTL logic LOW since all inputs and outputs are LVTTLcompatible.
	// 3. Provide stable CLOCK signal. Stable clock is defined as a signal cycling within timing
	// constraints specified for the clock pin. (maybe?)
	LPC_EMC->CONFIG  	= 0x00000000;

	// RBC
	/* 14 12 11:9 8:7
	 * 0  0  011  00 256 Mb (32Mx8), 4 banks, row length = 13, column length = 10
	 * 0  0  011  01 256 Mb (16Mx16), 4 banks, row length = 13, column length = 9
	 */
	LPC_EMC->DYNAMICCONFIG0    = 0<<14 | 0<<12 | 0<<11 | 1<<10 | 1<<9 | 0<<8 | 1<<7;

	// BRC
	/* 14 12 11:9 8:7
	 * 0  1  011  00 256 Mb (32Mx8), 4 banks, row length = 13, column length = 10
	 * 0  1  011  01 256 Mb (16Mx16), 4 banks, row length = 13, column length = 9
	 */
//	LPC_EMC->DYNAMICCONFIG0    = 0<<14 | 1<<12  | 0<<11 | 1<<10 | 1<<9 |  0<<8 | 1<<7;

	pclk = CGU_GetPCLKFrequency(CGU_PERIPHERAL_EMC);

	/*
	 * 1100000011
	 * RAS latency: Three EMC_CCLK cycles (POR reset value)
	 * CAS latency: Three EMC_CCLK cycles (POR reset value)
	 */
	// 1100000011
	LPC_EMC->DYNAMICRASCAS0    = 0x00000303;

	// Command delayed by 1/2 EMC_CCLK
	LPC_EMC->DYNAMICREADCONFIG = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */

	// Dynamic Memory Precharge Command Period register (tRP)
	// needs to be at least 20ns
	//	LPC_EMC->DYNAMICRP 			= 2;    // calculated from xls sheet for 120 MHz
//	LPC_EMC->DYNAMICRP         =  2;
	LPC_EMC->DYNAMICRP         =  NS2CLK(pclk, 20);

	/* Dynamic memory active to precharge command period register (tRAS)
	 * 0x0 - 0xE = n + 1 clock cycles. The delay is in EMC_CCLK cycles.
	 * 0xF = 16 clock cycles (POR reset value).
	 * needs to be between 45 and 100,000 ns
	 */
//	LPC_EMC->DYNAMICRAS        = 5;
	LPC_EMC->DYNAMICRAS        = NS2CLK(pclk, 45);

	// Dynamic Memory Self Refresh Exit Time register (tSREX, tXSR)
	// needs to be at least 75 ns
//	LPC_EMC->DYNAMICSREX       = 8;
	LPC_EMC->DYNAMICSREX       = NS2CLK(pclk, 75);;

	// Dynamic Memory Last Data Out to Active Time register (tAPR)
	// can't find tAPR value on datasheet. Using tRCD instead (Active-to-Read/Write delay). Should be 20ns
//	LPC_EMC->DYNAMICAPR        = 3; //0x00000005;
	LPC_EMC->DYNAMICAPR        = NS2CLK(pclk, 20);

	// Dynamic Memory Data In to Active Command Time register (tDAL)
	// should be at least 35ns
	LPC_EMC->DYNAMICDAL        = NS2CLK(pclk, 35);

	// Dynamic Memory Write Recovery Time register (tWR, tDPL, tRWL, or tRDL)
	// needs to be at least 14ns
//	LPC_EMC->DYNAMICWR 			= 3;
	LPC_EMC->DYNAMICWR         = NS2CLK(pclk, 14);

	// Dynamic Memory Active to Active Command Period register (tRC)
	// needs to be at least 68ns
//	LPC_EMC->DYNAMICRC         = 8;
	LPC_EMC->DYNAMICRC         = NS2CLK(pclk, 68);

	// Dynamic Memory Auto-refresh Period register (tRFC, tRC)
	// needs to be at least 68ns
//	LPC_EMC->DYNAMICRFC 		= 9;
	LPC_EMC->DYNAMICRFC        = NS2CLK(pclk, 68);

	// Dynamic Memory Exit Self Refresh register (tXSR)
	// needs to be at least 75ns
//	LPC_EMC->DYNAMICXSR 		= 8;
	LPC_EMC->DYNAMICXSR        = NS2CLK(pclk, 75);

	// Dynamic Memory Active Bank A to Active Bank B Time register (tRRD)
	// needs to be at least 14ns
//	LPC_EMC->DYNAMICRRD        = 2;
	LPC_EMC->DYNAMICRRD        = NS2CLK(pclk, 14);

	// Dynamic Memory Load Mode register to Active Command Time (tMRD)
	// needs to be at least 14ns
	LPC_EMC->DYNAMICMRD        = NS2CLK(pclk, 14);

	// 4. Wait at least 100μs prior to issuing any command other than a COMMAND INHIBIT or NOP.
	/*
	 * 110000011
	 * CE enable
	 * CLKOUT is on
	 * issue NOP
	 */
	/*
	 * 5. Starting at some point during this 100μs period, bring CKE HIGH. Continuing at least
		through the end of this period, 1 or more COMMAND INHIBIT or NOP commands
		must be applied.
	 */
	LPC_EMC->DYNAMICCONTROL    = EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_NOP);//0x00000183; /* Issue NOP command */

	TIM_Waitus(100);						   /* wait 100ms */

	// 6. Perform a PRECHARGE ALL command.
	/*
	 * 100000011
	 * CE enable
	 * CLKOUT is on
	 * issue precharge all command
	 */
	LPC_EMC->DYNAMICCONTROL    = EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_PRECHARGE_ALL);//0x00000103; /* Issue PALL command */

	/* 7. Wait at least tRP time; during this time NOPs or DESELECT commands must be given.
		All banks will complete their precharge, thereby placing the device in the all banks
		idle state. (tRP = 15ns)
		*/
	TIM_Waitus(200);
	LPC_EMC->DYNAMICCONTROL    =  EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_NOP);//0x00000183; /* Issue NOP command */

	/* 8. Issue an AUTO REFRESH command.
	 * Is this actually an auto-refresh command? This might only set up the register...?
	 */
	LPC_EMC->DYNAMICREFRESH    = EMC_SDRAM_REFRESH(pclk, 64000000/8192);

	/* 9. Wait at least tRFC time, during which only NOPs or COMMAND INHIBIT commands
		are allowed. (tRFC = 66ns)
	 */
	TIM_Waitus(200);						   /* wait 100ms */

//	tmpclk = (uint64_t)15625*(uint64_t)pclk/1000000000/16;

	/* 10. Issue an AUTO REFRESH command.
	 */
//	LPC_EMC->DYNAMICREFRESH 	= 118;
//	LPC_EMC->DYNAMICREFRESH    = tmpclk; /* ( n * 16 ) -> 736 clock cycles -> 15.330uS at 48MHz <= 15.625uS ( 64ms / 4096 row ) */

	// 11. Wait at least tRFC time, during which only NOPs or COMMAND INHIBIT commands are allowed. (tRFC = 66ns)
//	TIM_Waitus(100);						   /* wait 100ms */

	/*
	12. The SDRAM is now ready for mode register programming. Because the mode register
	will power up in an unknown state, it should be loaded with desired bit values prior to
	applying any operational command. Using the LMR command, program the mode
	register. The mode register is programmed via the MODE REGISTER SET command
	with BA1 = 0, BA0 = 0 and retains the stored information until it is programmed again
	or the device loses power. Not programming the mode register upon initialization will
	result in default settings which may not be desired. Outputs are guaranteed High-Z
	after the LMR command is issued. Outputs should be High-Z already before the LMR
	command is issued.
	 * 10000011
	 * CE enable
	 * CLKOUT is on
	 * issue SDRAM Mode command
	 */
	LPC_EMC->DYNAMICCONTROL    = EMC_CE_ENABLE | EMC_CS_ENABLE | EMC_INIT(EMC_MODE);//0x00000083; /* Issue MODE command */

	// 13. Wait at least tMRD time, during which only NOP or DESELECT commands are allowed. (tMRD = 2 emc clock cycles)
//	TIM_Waitus(3);						   /* wait 3ms */

	//Issue SDRAM MODE command
	/* The SDRAM mode register is loaded in two steps:
	 * 1. Use the DYNAMICCONTROL register to issue a set mode command.
	 * 2. When the SDRAM is in the set mode state, issue an SDRAM read from an address
	 * 		specific to the selected mode and the SDRAM memory organization.
	 *
	 * The read address is calculated as follows:
	 * Determine the mode register content MODE:
	 * – For a single 16-bit external SDRAM chip set the burst length to 8. For a single
	 * 		32-bit SDRAM chip set the burst length to 4.
	 * – Select the sequential mode.
	 * – Select latency mode = 2 or 3.
	 * – Select Write burst mode = 0.
	 * Determine the shift value OFFSET to shift the mode register content by. This shift
	 * value depends on the SDRAM device organization and it is calculated as:
	 * 		OFFSET = number of columns + total bus width + bank select bits (RBC mode)
	 * 		OFFSET = number of columns + total bus width (BRC mode)
	 * Select the SDRAM memory mapped address DYCS0.
	 * The SDRAM read address is ADDRESS = DYCS0 + (MODE << OFFSET).
	 */
	//Timing for 48/60/72MHZ Bus
	// 0  0  011  01 256 Mb (16Mx16), 4 banks, row length = 13, column length = 9
	// offset = 12 = 9 (number of columns) + 1 (1 data bit at a time) + 2 (4 banks, 2 bits used to select)

	// BRC mode
//	temp = *((volatile uint32_t *)(SDRAM_ADDR_BASE | (3<<4 | 3)<<10)); /* 8 burst, 3 CAS latency */

	// RBC mode
	temp = *((volatile uint32_t *)(SDRAM_ADDR_BASE | (3<<4 | 3)<<12)); /* 8 burst, 3 CAS latency */
//	temp = temp;

//	for(int i = 128; i; i--); // wait 128 clk cycles
	LPC_EMC->DYNAMICCONTROL    = 0x00000000; /* Issue NORMAL command */

	//[re]enable buffers
	LPC_EMC->DYNAMICCONFIG0    |= 1<<19;
}
