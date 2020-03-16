/*
 * Copyright (c) 2019, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * AES128 CBC encryption & decryption using mbedtls aes library.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "crypto.h"
#include "log.h"
#include "mbedtls/aes.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define AES128_KEY_BIT_LENGTH 	128
#define AES_BLOCK_SIZE 			16

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static void prvAes128SetKey( mbedtls_aes_context *xContext, eCryptoMode_t eMode, const uint8_t *pucKey );

/* Private Variables ----------------------------------------*/

void vAes128Crypt( eCryptoMode_t eMode, const uint8_t pucKey[AES128_KEY_LENGTH], uint8_t pucInitVector[AES128_IV_LENGTH], const uint8_t *pucInput, uint8_t ucDataBlocksNum, uint8_t *pucOutput )
{
	mbedtls_aes_context xAesContext;

	/* Initialise cipher context */
	mbedtls_aes_init( &xAesContext );
	/* Set key */
	prvAes128SetKey( &xAesContext, eMode, pucKey );

	mbedtls_aes_crypt_cbc( &xAesContext, eMode, ucDataBlocksNum * AES_BLOCK_SIZE, pucInitVector, pucInput, pucOutput );
}

/*-----------------------------------------------------------*/

void prvAes128SetKey( mbedtls_aes_context *xContext, eCryptoMode_t eMode, const uint8_t *pucKey )
{
	if ( eMode == ENCRYPT ) {
		mbedtls_aes_setkey_enc( xContext, pucKey, AES128_KEY_BIT_LENGTH );
	}
	else {
		mbedtls_aes_setkey_dec( xContext, pucKey, AES128_KEY_BIT_LENGTH );
	}
}

/*-----------------------------------------------------------*/
