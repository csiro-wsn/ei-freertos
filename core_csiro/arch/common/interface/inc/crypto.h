/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 */
#ifndef __CSIRO_CORE_AES128_CRYPTO
#define __CSIRO_CORE_AES128_CRYPTO
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* Module Defines -------------------------------------------*/
// clang-format off

#define AES128_BLOCK_LENGTH		16

#define AES128_KEY_LENGTH  		AES128_BLOCK_LENGTH
#define AES128_IV_LENGTH 		AES128_BLOCK_LENGTH

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eCryptoMode_t {
	DECRYPT = 0x00,
	ENCRYPT
} eCryptoMode_t;

/* Function Declarations ------------------------------------*/

/**@brief Encrypt/decrypt a binary array using AES128 in CBC mode & mbedtls AES library
 *
 * @param[in] eMode                 Crypto mode
 * @param[in] pucKey	            Encryption/decryption key depending on eMode, must be 16-byte long
 * @param[in] pucInitVector         Initialisation vector (Updated after use), must be same size as pucKey
 * @param[in] pucInput              Data to encrypt/decrypt depending on eMode
 * @param[in] ucDataBlocksNum       Size of pucData in 16-byte units
 * @param[out] pucOutput            Output buffer, same size as pucData 
 *
 * @note     pucInitVector is updated & overwritten by call to mbedtls crypting function
 * 
 * @retval   None    
 */
void vAes128Crypt( eCryptoMode_t eMode, const uint8_t pucKey[AES128_KEY_LENGTH], uint8_t pucInitVector[AES128_IV_LENGTH], const uint8_t *pucInput, uint8_t ucDataBlocksNum, uint8_t *pucOutput );

#endif /* __CSIRO_CORE_AES128_CRYPTO */
