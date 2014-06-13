#include "neopixel.h" 

void initiateTimer();
void stopTimer();

#define NEOPIXEL_BYTE_FREQ 2400000

uint8_t buf[10] = {0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010, 0b10101010 };
uint8_t byte = 0;
uint8_t bit = 0;
uint8_t frame = 0;
uint8_t data_pin = 0;

void basicTest (uint8_t pin) {

  hw_digital_output(pin);

  data_pin = pin;

  initiateTimer();
}

void writePixels() {

  switch (frame) {
    case 0:
      hw_digital_write(data_pin, 1);
      frame++;
      break;
    case 1:
      if (!(buf[byte] & (1 << bit))) {
        hw_digital_write(data_pin, 0);
      }
      frame++;
      break;
    case 2:
      hw_digital_write(data_pin, 0);
      frame = 0;
      if (bit == 7) {
        stopTimer();
        bit = 0;
        byte++;
        if (byte < 10) initiateTimer();
      } 
      else {
        bit++;
      }
      break;
  }
}

void RIT_IRQHandler(void)
{
  RIT_GetIntStatus(LPC_RITIMER);

  writePixels();
}

void initiateTimer() {
  // INIT
  LPC_RITIMER->COMPVAL = 0xFFFFFFFF;
  LPC_RITIMER->MASK  = 0x00000000;
  LPC_RITIMER->CTRL  = 0x0C;
  LPC_RITIMER->COUNTER = 0x00000000;

  // CONFIG
  // Get PCLK value of RIT
  uint32_t clock_rate, cmp_value;
  clock_rate = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M3CORE);

   // calculate compare value for RIT to generate interrupt at
  cmp_value = (clock_rate / NEOPIXEL_BYTE_FREQ);
  LPC_RITIMER->COMPVAL = cmp_value;

  /* Set timer enable clear bit to clear timer to 0 whenever
   * counter value equals the contents of RICOMPVAL
   */
  LPC_RITIMER->CTRL |= (1<<1);

  NVIC_EnableIRQ(RITIMER_IRQn);
}

void stopTimer() {
  RIT_Cmd(LPC_RITIMER, DISABLE);

  NVIC_DisableIRQ(RITIMER_IRQn);
}