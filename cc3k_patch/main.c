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

void __attribute__ ((interrupt)) GPIO0_IRQHandler(void)
{
	validirqcount++;
	if (GPIO_GetIntStatus(0))
	{
		CC3K_IRQ_FLAG = 1;
		hw_digital_write(CC3K_ERR_LED, 1);
		GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, 0);
		enqueue_system_event(SPI_IRQ_CALLBACK_EVENT, 0);
	}
}


void __attribute__ ((interrupt)) GPIO1_IRQHandler(void)
{
	SPI_IRQ();
}

void smart_config_task ();

/**
 * Smartconfig Button
 */

void __attribute__ ((interrupt)) GPIO3_IRQHandler(void)
{
  if (GPIO_GetIntStatus(3))
  {
    // if (smartconfig_process == 0) {
    //   smartconfig_process = 1;
    //   smart_config_task();
    // }
    GPIO_ClearInt(TM_INTERRUPT_MODE_RISING, 3);
    GPIO_ClearInt(TM_INTERRUPT_MODE_FALLING, 3);
  }
}


// stub so that hw_interrupt compiles
void script_msg_queue (char *type, void* data, size_t size) {
	
	(void) type;
	(void) data;
	(void) size;
}

// stub so that hw_interrupt compiles
void enqueue_system_event(event_callback callback, unsigned data) {
	(void) callback;
	(void) data;
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

	hw_interrupt_enable(0, CC3K_IRQ, TM_INTERRUPT_MODE_FALLING);
	NVIC_SetPriority(PIN_INT1_IRQn, ((0x03<<3)|0x02));
	NVIC_EnableIRQ(PIN_INT1_IRQn);

	hw_interrupt_enable(3, CC3K_CONFIG, TM_INTERRUPT_MODE_FALLING);
  	NVIC_SetPriority(PIN_INT3_IRQn, ((0x03<<3)|0x02)); // TODO: is this the wrong priority?
  	NVIC_EnableIRQ(PIN_INT3_IRQn);

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