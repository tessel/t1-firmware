#include "hw.h"
#include "PatchProgrammer.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_scu.h"
#include "tessel.h"
#include "lpc18xx_gpio.h"
#include "spi_flash.h"
#include "bootloader.h"

#include <string.h>

/**
 * Custom awful CC3000 code
 */

volatile int validirqcount = 0;
volatile int netconnected = 0;

volatile uint8_t CC3K_IRQ_FLAG = 0;

void SPI_IRQ_CALLBACK_EVENT (unsigned no)
{
	(void) no;
	if (CC3K_IRQ_FLAG) {
		hw_digital_write(CC3K_ERR_LED, 0);
		CC3K_IRQ_FLAG = 0;
		SPI_IRQ();
	}
}

void set_cc3k_irq_flag (uint8_t val) {
	CC3K_IRQ_FLAG = val;
}

uint8_t get_cc3k_irq_flag () {
	return CC3K_IRQ_FLAG;
}

void __attribute__ ((interrupt)) GPIO7_IRQHandler(void)
{
	validirqcount++;
	if (GPIO_GetIntStatus(CC3K_GPIO_INTERRUPT))
	{
		CC3K_IRQ_FLAG = 1;
		hw_digital_write(CC3K_ERR_LED, 1);
		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, CC3K_GPIO_INTERRUPT);
	}
}

// stub so that hw_interrupt compiles
void script_msg_queue (char *type, void* data, size_t size) {
	
	(void) type;
	(void) data;
	(void) size;
}

int main (void){
	__disable_irq();
	SystemInit();
	CGU_Init();
	
	// set up pins
	hw_digital_output(LED1);
	hw_digital_output(LED2);
	hw_digital_output(CC3K_CONN_LED);
	hw_digital_output(CC3K_ERR_LED);

	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

	// do not boot cc3k
	hw_digital_output(CC3K_SW_EN);
	hw_digital_write(CC3K_SW_EN, 0);
	// hw_digital_input(CC3K_IRQ);

	hw_interrupt_enable(CC3K_GPIO_INTERRUPT, CC3K_IRQ, TM_INTERRUPT_MODE_FALLING);
	NVIC_SetPriority(PIN_INT1_IRQn, ((0x03<<3)|0x02));
	NVIC_EnableIRQ(PIN_INT1_IRQn);

	// wait a while
	hw_wait_ms(500);

	// do the patch
	patch_programmer();

	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 1);

	jump_to_flash(FLASH_FW_ADDR, 0);

	return 0;
}

/*******************************************************************************
* @brief    Reports the name of the source file and the source line number
*         where the CHECK_PARAM error has occurred.
* @param[in]  file Pointer to the source file name
* @param[in]    line assert_param error line source number
* @return   None
*******************************************************************************/
void check_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	(void) file;
	(void) line;

	/* Infinite loop */
	while(1);
}