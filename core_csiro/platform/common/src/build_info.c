/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "application_images.h"

#include "application.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

const xBuildInfo_t xBuildInfo ATTR_SECTION( ".buildinfo" ) = {
	.ulKey			= BUILD_INFO_VALID_KEY,
	.usVersionMajor = APP_MAJOR,
	.usVersionMinor = APP_MINOR,
	.ulBuildtime	= BUILD_TIMESTAMP,
	.pucGitHash		= COMMIT_HASH
};

/*-----------------------------------------------------------*/
