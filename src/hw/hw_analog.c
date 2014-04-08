// MIT

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lpc18xx_dac.h"
#include "lpc18xx_adc.h"
#include "variant.h"
#include "hw.h"

/*
static int _readResolution = 10;
static int _writeResolution = 8;

void analogReadResolution(int res) {
	_readResolution = res;
}

void analogWriteResolution(int res) {
	_writeResolution = res;
}

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to) {
	if (from == to)
		return value;
	if (from > to)
		return value >> (from-to);
	else
		return value << (to-from);
}

eAnalogReference analog_reference = AR_DEFAULT;

void analogReference(eAnalogReference ulMode)
{
	analog_reference = ulMode;
}
*/

uint32_t hw_analog_read (uint32_t ulPin)
{
	uint32_t ulValue = 0;
	uint32_t ulChannel = g_APinDescription[ulPin].analogChannel;
//	uint32_t ulADC_Type;

	// switch to ADC if it's not there by default
    if (g_APinDescription[ulPin].alternate == ADC_MODE){
    	// set it up as a gpio and then use adc
    	hw_digital_input(ulPin);
    	LPC_SCU->ENAIO0 |= (1 << g_APinDescription[ulPin].alternate_func);
	}

    // enable
	ADC_Init(LPC_ADC0, 200000, 10);//LPC_ADC0, 200000, 10);
  ADC_IntConfig(LPC_ADC0, ADC_ADGINTEN, DISABLE); // global interrupt is enabled by default
  NVIC_EnableIRQ(ADC0_IRQn);                      // enable interupt in NVIC

	ADC_ChannelCmd(LPC_ADC0, ulChannel, ENABLE);

	// start
	ADC_StartCmd(LPC_ADC0, ADC_START_NOW);
	// Wait for DONE bit set
	while (ADC_ChannelGetStatus(LPC_ADC0, ulChannel, ADC_DATA_DONE) != SET);

  // Read result
	ulValue = ADC_ChannelGetData(LPC_ADC0, ulChannel);
	// Disable channel
  ADC_ChannelCmd(LPC_ADC0, ulChannel, DISABLE);

  // deactivate
  if (g_APinDescription[ulPin].alternate == ADC_MODE){
    // set it up as a gpio and then use adc
//    hw_digital_input(ulPin);
    LPC_SCU->ENAIO0 |= (0 << g_APinDescription[ulPin].alternate_func);
  }

  return ulValue;
}


// Right now, PWM output only works on the pins with
// hardware support.  These are defined in the appropriate
// pins_*.c file.  For the rest of the pins, we default
// to digital output.

int hw_analog_write (uint32_t ulPin, float ulValue)
{
	// channel 0 is actually the only channel that can do straight up dac
	if (g_APinDescription[ulPin].analogChannel != ADC_CHANNEL_0) {
		return 1;
	}

	//uint32_t attr = g_APinDescription[ulPin].ulPinAttribute;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_SetBias(LPC_DAC, 1);// set to low bias for now
	LPC_SCU->ENAIO2 |= 1;	 // Enable analog function
	DAC_SetDMATimeOut(LPC_DAC,0);

	DAC_Init(LPC_DAC);
	DAC_ConfigDAConverterControl(LPC_DAC, &DAC_ConverterConfigStruct);
	DAC_UpdateValue(LPC_DAC, ulValue);
	return 0;	
}


#ifdef __cplusplus
}
#endif
