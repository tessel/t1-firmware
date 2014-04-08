#include "lpc18xx_cgu.h"
#include "lpc18xx_scu.h"
#include "hw.h"
#include "variant.h"
#include "spi_flash.h"
#include "bootloader.h"

#include <string.h>

int main() {
	__disable_irq();
	SystemInit();
	CGU_Init();

	hw_digital_output(LED1);
	hw_digital_output(LED2);
	hw_digital_output(CC3K_CONN_LED);
	hw_digital_output(CC3K_ERR_LED);

	hw_digital_write(LED1, 0);
	hw_digital_write(LED2, 0);
	hw_digital_write(CC3K_CONN_LED, 0);
	hw_digital_write(CC3K_ERR_LED, 0);

	SCnSCB->ACTLR &= ~2;
	spiflash_init();

	// erase user space code
	spiflash_erase_sector(FLASH_FS_START);
	
	spiflash_mem_mode();

	SCnSCB->ACTLR |= 2;

	jump_to_flash(FLASH_FW_ADDR, 0);
}


void check_failed(uint8_t *file, uint32_t line) {
	(void) file;
	(void) line;
  /* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while(1);
}
