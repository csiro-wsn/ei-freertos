/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: application_images.h
 * Creation_Date: 05/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Manager for application images stored in external flash
 * 
 */
#ifndef __CSIRO_CORE_APPLICATION_IMAGES
#define __CSIRO_CORE_APPLICATION_IMAGES
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define BUILD_INFO_VALID_KEY    0x76548ABC

// clang-format on

/* Type Definitions -----------------------------------------*/

/**@brief Application Image Information 
 * 
 * A static instantiation of this struct is expected to be stored at a known location in Flash
 * The expected address can be queried through pvApplicationBuildInfoOffset
*/
typedef struct xBuildInfo_t
{
	uint32_t ulKey;			 /**< Validity Key*/
	uint16_t usVersionMajor; /**< Major version number */
	uint16_t usVersionMinor; /**< Minor version number */
	uint32_t ulBuildtime;	/**< Image buildtime (Unix Timestamp) */
	uint8_t  pucGitHash[20]; /**< Images pacp-freertos commit hash */
} ATTR_PACKED xBuildInfo_t;

/* Function Declarations ------------------------------------*/

/**@brief Number of application images supported by device
 *
 * @note            Number of application image slots are defined in device constants, not per application
 * 
 * @retval			Number of application image slots
 */
uint32_t ulNumApplicationImages( void );

/**@brief Size of a single application image in bytes
 *
 * @retval			Size of an application image
 */
uint32_t ulApplicationImageSize( void );

/**@brief Expected address of the xBuildInfo_t struct in flash for application images
 * 
 * @retval	        Flash address of xBuildInfo_t
 */
void *pvApplicationBuildInfoOffset( void );

/**@brief Get local xBuildInfo_t struct
 * 
 * @retval	        Local xBuildInfo_t address
 */
xBuildInfo_t *pxLocalBuildInfo( void );

#endif /* __CSIRO_CORE_APPLICATION_IMAGES */
