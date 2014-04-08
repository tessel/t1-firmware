/*****************************************************************************
 * $Id: lpc18xx_AES_OTP.h 2011-12-27
*//**
* @file		lpc18xx_AES_OTP.h
* @brief	Provides access to AES and OTP routines contained within on-chip ROM 
*              of LPC18xx devices.
* @version	1.0
* @date		27. Dec. 2011
* @author	NXP MCU SW Application Team

 * Copyright(C) 2011, NXP Semiconductor
 * All rights reserved.
 *
 *****************************************************************************
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
 *****************************************************************************/
 
/* Peripheral group ----------------------------------------------------------- */
/** @defgroup AES_OTP AES_OTP
 * @ingroup LPC1800CMSIS_FwLib_Drivers
 * @{
 */
 
#ifndef __LPC18XX_AES_OTP_H
#define	__LPC18XX_AES_OTP_H

/* Includes ------------------------------------------------------------------- */
#include "lpc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif



/* Public Macros -------------------------------------------------------------- */
/** @defgroup AES_OTP_Public_Macros  AES_OTP Public Macros
 * @{
 */
/** AES Mode input for aes_SetMode function */
#define AES_API_MODE_ENCODE_ECB		0
#define AES_API_MODE_DECODE_ECB		1
#define AES_API_MODE_ENCODE_CBC		2
#define AES_API_MODE_DECODE_CBC		3

/** A table of pointers to the AES functions contained in ROM is located at the
   address contained at this location 
*/
#define AES_FUNCTION_TABLE_PTR_ADDR			0x10400108UL
/** A table of pointers to the OTP functions contained in ROM is located at the
   address contained at this location
*/
#define OTP_FUNCTION_TABLE_PTR_ADDR			0x10400104UL

/**
 * @}
 */

/* Public Types --------------------------------------------------------------- */
/** @defgroup AES_OTP_Public_Types AES_OTP Public Types
 * @{
 */
 
/**
 * @brief OTP Boot Source Type definition
 */
typedef enum {
  OTP_BOOTSRC_PINS,			/*!< Boot from external pins */
  OTP_BOOTSRC_UART0,		/*!< Boot from UART 0 */
  OTP_BOOTSRC_SPIFI,		/*!< Bott from SPIFI */
  OTP_BOOTSRC_EMC8,			/*!< Boot from EMC 8 bits */
  OTP_BOOTSRC_EMC16,		/*!< Boot from EMC 16 bits */
  OTP_BOOTSRC_EMC32,		/*!< Boot from EMC 32 bits */
  OTP_BOOTSRC_USB1,			/*!< Boot from USB 0 */
  OTP_BOOTSRC_USB2,			/*!< Boot from USB 1*/
  OTP_BOOTSRC_SPI,			/*!< Boot from SPI(SSP) */
  OTP_BOOTSRC_UART3			/*!< Boot from UART 3 */
} bootSrc_e;

#pragma pack(4)

/**
 * @brief AES API table Type definition
 */
typedef struct {
	uint32_t (*aes_Init)(void);						/**< Initialize AES engine.*/
	uint32_t (*aes_SetMode)(uint32_t u32Mode);		/**< Define AES engine operation mode*/
	uint32_t (*aes_LoadKey1)(void);					/**< Load 128-bit AES user key 1 */
	uint32_t (*aes_LoadKey2)(void);					/**< Load 128-bit AES user key 2 */
	uint32_t (*aes_LoadKeyRNG)(void);				/**< Load randomly generated key in AES engine.
													To update the RNG and load a new random number
													the API call otp_GenRand should be used before */
													
	uint32_t (*aes_LoadKeySW)(uint8_t *pau8Key);	/**< Load 128-bit AES software defined user key */
	uint32_t (*aes_LoadIV_SW)(uint8_t *pau8Vector);	/**< Load 128-bit AES initialization vector */
	uint32_t (*aes_LoadIV_IC)(void);				/**< Load 128-bit AES IC specific initialization vector
													which is used to decrypt a boot image */
	uint32_t (*aes_Operate)(uint8_t *pau8Output, uint8_t *pau8Input, uint32_t u32Size);	/**< Performs the AES decryption
													after the AES mode has been set using aes_SetMode and the appropriate
													keys and init vectors have been loaded */
	uint32_t (*aes_ProgramKey1)(uint8_t *pau8Key);	/**< Programs 128-bit AES key in OTP */
	uint32_t (*aes_ProgramKey2)(uint8_t *pau8Key);	/**< Programs 128-bit AES key in OTP */
} AES_API_TABLE_T;

/**
 * @brief OTP API Table Type definition
 */
typedef struct {
	uint32_t (* otp_Init)(void);							/**< Initializes OTP controller */
	uint32_t (* otp_ProgBootSrc)(bootSrc_e BootSrc);		/**< Programs boot source */
	uint32_t (* otp_ProgJTAGDis)(void);						/**< Set JTAG disable. This command disables
															JTAG only when device is AES capable */
	uint32_t (* otp_ProgUSBID)(uint32_t ProductID, uint32_t VendorID);/**< Program USB ID */
	uint32_t Reserved[3];
	uint32_t (* otp_ProgGP0)(uint32_t Data, uint32_t Mask);	/**< Programs the general purpose OTP memory GP0.
															Use only if the device is not AES capable. */
	uint32_t (* otp_ProgGP1)(uint32_t Data, uint32_t Mask);	/**< Programs the general purpose OTP memory GP1.
															Use only if the device is not AES capable. */
	uint32_t (* otp_ProgGP2)(uint32_t Data, uint32_t Mask);	/**< Programs the general purpose OTP memory GP2.
															Use only if the device is not AES capable. */
	uint32_t (* otp_ProgKey1)(uint8_t *key);				/**< Program AES key1 */
	uint32_t (* otp_ProgKey2)(uint8_t *key);				/**< Program AES key2 */
	uint32_t (* otp_GenRand)(void);							/**< Generate new random number using hardware
															Random Number Generator (RNG). */
} OTP_API_TABLE_T;

/**
 * @}
 */

/** Points to AES ROM API */
#define AESLib (*((AES_API_TABLE_T**)AES_FUNCTION_TABLE_PTR_ADDR))
/** Points to OTP ROM API */
#define OTPLib (*((OTP_API_TABLE_T**)OTP_FUNCTION_TABLE_PTR_ADDR))

#ifdef __cplusplus
}
#endif

#endif /* end __LPC18XX_AES_OTP_H */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
