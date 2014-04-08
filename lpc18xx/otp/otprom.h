/**********************************************************************
* $Id$		otprom.h		2011-04-21
*//**
* @file		otprom.h
* @brief	Bootloader OTP component header file 
* @version	1.0
* @date		21. April. 2011
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/

#ifndef OTPROM_H_
#define OTPROM_H_

/* OTP register map */
#define OTP_BASE 0x40045000

uint32_t otp_Init(void);

typedef enum {
  OTP_BOOTSRC_PINS,
  OTP_BOOTSRC_UART0,
  OTP_BOOTSRC_SPIFI,
  OTP_BOOTSRC_EMC8,
  OTP_BOOTSRC_EMC16,
  OTP_BOOTSRC_EMC32,
  OTP_BOOTSRC_USB1,
  OTP_BOOTSRC_USB2,
  OTP_BOOTSRC_SPI,
  OTP_BOOTSRC_UART3
} bootSrc_e;


#define LPC_OK 0x00000000
#define ERR_FAILED 0xFFFFFFFF
#define ERR_API_INVALID_PARAMS 0x00010001
#define ERR_API_INVALID_PARAM1 0x00010002
#define ERR_API_INVALID_PARAM2 0x00010003
#define ERR_API_INVALID_PARAM3 0x00010004
#define ERR_API_MOD_INIT 0x00010005
#define ERR_OTP_WR_ENABLE_INVALID 0x00070001
#define ERR_OTP_SOME_BITS_ALREADY_PROGRAMMED 0x00070002
#define ERR_OTP_ALL_DATA_OR_MASK_ZERO 0x00070003
#define ERR_OTP_WRITE_ACCESS_LOCKED 0x00070004
#define ERR_OTP_READ_DATA_MISMATCH 0x00070005
#define ERR_OTP_USB_ID_ENABLED 0x00070006
#define ERR_SEC_AES_KEY_ALREADY_PROGRAMMED 0x00030001


extern uint32_t (* otp_ProgBootSrc)(bootSrc_e BootSrc);
extern uint32_t (* otp_ProgJTAGDis)(void);
extern uint32_t (* otp_ProgUSBID)(uint32_t ProductID, uint32_t VendorID);
extern uint32_t (* otp_ProgGP0)(uint32_t Data, uint32_t Mask);
extern uint32_t (* otp_ProgGP1)(uint32_t Data, uint32_t Mask);
extern uint32_t (* otp_ProgGP2)(uint32_t Data, uint32_t Mask);
extern uint32_t (* otp_ProgKey1)(uint8_t *key);
extern uint32_t (* otp_ProgKey2)(uint8_t *key);
extern uint32_t (* otp_GenRand)(void);

#endif /*OTPROM_H_*/
