/*
 * i2c.cpp
 *
 *  Created on: Jul 8, 2013
 *      Author: tim
 */

#include <stddef.h>
#include <stdint.h>

#include "hw.h"
#include "lpc18xx_i2c.h"
#include "lpc18xx_cgu.h"
#include "lpc18xx_gpio.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_libcfg.h"

#ifdef __cplusplus
extern "C" {
#endif

void hw_i2c_initialize (uint32_t port)
{
	// Initialize Slave I2C peripheral
  if (port == (uint32_t) LPC_I2C1){
    scu_pinmux(2, 3, MD_PLN_FAST, FUNC1);
    scu_pinmux(2, 4, MD_PLN_FAST, FUNC1);
  }
	I2C_Init((LPC_I2Cn_Type*) port, 25000);
}

void hw_i2c_enable (uint32_t port, uint8_t mode)
{
  // mode: I2C_MASTER_MODE, I2C_SLAVE_MODE, I2C_GENERAL_MODE
	// Enable Master/Slave I2C operation
	I2C_Cmd((LPC_I2Cn_Type*) port, (I2C_Mode) mode, ENABLE);
}

void hw_i2c_disable (uint32_t port)
{
  // disable Master/Slave I2C operation
  // mode does not matter
	I2C_Cmd((LPC_I2Cn_Type*) port, I2C_MASTER_MODE, DISABLE);
}

void hw_i2c_set_slave_addr (uint32_t port, uint8_t slave_addr)
{
  // port: LPC_I2C1, LPC_I2C0
  I2C_OWNSLAVEADDR_CFG_Type OwnSlavAdr;
  OwnSlavAdr.GeneralCallState = DISABLE;
  OwnSlavAdr.SlaveAddrChannel= 0;
  OwnSlavAdr.SlaveAddrMaskValue = 0xFF;
  OwnSlavAdr.SlaveAddr_7bit = slave_addr;
  I2C_SetOwnSlaveAddr((LPC_I2Cn_Type*) port, &OwnSlavAdr);
}

int hw_i2c_slave_transfer (uint32_t port, const uint8_t *txbuf, size_t txbuf_len, uint8_t *rxbuf, size_t rxbuf_len)
{
  I2C_S_SETUP_Type transferSCfg;
  transferSCfg.tx_data = (uint8_t*) txbuf;
  transferSCfg.tx_length = txbuf_len;
  transferSCfg.rx_data = rxbuf;
  transferSCfg.rx_length = rxbuf_len;
  transferSCfg.tx_count = 0;
  transferSCfg.rx_count = 0;

  return I2C_SlaveTransferData((LPC_I2Cn_Type*) port, &transferSCfg, I2C_TRANSFER_POLLING);
}

int hw_i2c_slave_send (uint32_t port, const uint8_t *txbuf, size_t txbuf_len)
{
  return hw_i2c_slave_transfer(port, txbuf, txbuf_len, NULL, 0);
}

int hw_i2c_slave_receive (uint32_t port, uint8_t *rxbuf, size_t rxbuf_len)
{
  return hw_i2c_slave_transfer(port, NULL, 0, rxbuf, rxbuf_len);
}

int hw_i2c_master_transfer (uint32_t port, uint32_t addr, const uint8_t *txbuf, size_t txbuf_len, uint8_t *rxbuf, size_t rxbuf_len)
{
	I2C_M_SETUP_Type transferMCfg;
	transferMCfg.sl_addr7bit = addr;
	transferMCfg.tx_data = (uint8_t*) txbuf;
	transferMCfg.tx_length = txbuf_len;
	transferMCfg.rx_data = rxbuf;
	transferMCfg.rx_length = rxbuf_len;
	transferMCfg.retransmissions_max = 3;
	transferMCfg.tx_count = 0;
	transferMCfg.rx_count = 0;
	transferMCfg.retransmissions_count = 0;
	return I2C_MasterTransferData((LPC_I2Cn_Type*) port, &transferMCfg, I2C_TRANSFER_POLLING);
}

int hw_i2c_master_send (uint32_t port, uint32_t addr, const uint8_t *txbuf, size_t txbuf_len)
{
	return hw_i2c_master_transfer(port, addr, txbuf, txbuf_len, NULL, 0);
}

int hw_i2c_master_receive (uint32_t port, uint32_t addr, uint8_t *rxbuf, size_t rxbuf_len)
{
	return hw_i2c_master_transfer(port, addr, NULL, 0, rxbuf, rxbuf_len);
}

#ifdef __cplusplus
}
#endif
