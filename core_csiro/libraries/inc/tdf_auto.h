/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 * All rights reserved.
 *
 * Filename: tdf_auto.h
 *
 * Autogenerated by tdftool
 * 
 * DO NOT EDIT
 */

#ifndef __CORE_CSIRO_LIBRARIES_TDF_AUTO
#define __CORE_CSIRO_LIBRARIES_TDF_AUTO
// clang-format off

/* Includes -------------------------------------------------*/

#include "stdint.h"

#include "tdf_struct.h"

/* Type Definitions -----------------------------------------*/

typedef enum eTdfIds {
    TDF_BATTERY_VOLTAGE                     = 8,
    TDF_BATTERY_CURRENT                     = 9,
    TDF_SOLAR_VOLTAGE                       = 10,
    TDF_SOLAR_CURRENT                       = 11,
    TDF_CUMULATIVE_SOLAR_CURRENT            = 12,
    TDF_ONBOARD_TEMP                        = 13,
    TDF_RAW_ADC                             = 14,
    TDF_RAW_ADC_N                           = 15,
    TDF_DATETIME_MS                         = 16,
    TDF_DATETIME                            = 17,
    TDF_LOCATION_XYZ                        = 18,
    TDF_LOCATION_XY32                       = 19,
    TDF_LOCATION_XY16                       = 20,
    TDF_APP_VERSION                         = 21,
    TDF_BOOTLOADER_VERSION                  = 22,
    TDF_RESET_COUNT                         = 23,
    TDF_REASON_FOR_LAST_RESET               = 24,
    TDF_MAC_RX_COUNT                        = 25,
    TDF_MAC_TX_COUNT                        = 26,
    TDF_MAC_RX_DROP_COUNT                   = 27,
    TDF_MAC_TX_ERR_COUNT                    = 28,
    TDF_MAC_TX_DELAY_MEAN                   = 29,
    TDF_MAC_TX_DELAY_MAX                    = 30,
    TDF_MAC_TIMEOUT_COUNT                   = 31,
    TDF_MAC_ACK_TIMEOUT_COUNT               = 32,
    TDF_MAC_ACK_ERR_COUNT                   = 33,
    TDF_NW_TX_QFULL_COUNT                   = 34,
    TDF_NW_RX_QFULL_COUNT                   = 35,
    TDF_NW_WORK_QFULL_COUNT                 = 36,
    TDF_NW_RETRY_COUNT                      = 37,
    TDF_NW_TX_DROP_COUNT                    = 38,
    TDF_NW_SYNC_LOST_COUNT                  = 39,
    TDF_NW_PKT_FORWARD_COUNT                = 40,
    TDF_NW_NODEID                           = 41,
    TDF_NW_STATS1                           = 42,
    TDF_NW_STATS2                           = 43,
    TDF_NW_STATS3                           = 44,
    TDF_NW_BIGSEQ                           = 45,
    TDF_NW_ACKSENDS                         = 46,
    TDF_NW_TXQUAL                           = 47,
    TDF_NW_RXQUAL                           = 48,
    TDF_NW_HOPCOUNT                         = 49,
    TDF_NW_NEXTHOP                          = 50,
    TDF_NW_TRACEROUTE                       = 51,
    TDF_CALIBRATION_DATA                    = 52,
    TDF_MOISTURE_ECHO_1                     = 53,
    TDF_MOISTURE_ECHO_N                     = 54,
    TDF_ACC_XYZ                             = 55,
    TDF_MAG_XYZ                             = 56,
    TDF_GYRO_XYZ                            = 57,
    TDF_WATER_PH                            = 58,
    TDF_WATER_REDOX                         = 59,
    TDF_WATER_DO                            = 60,
    TDF_WATER_TEMP                          = 61,
    TDF_WATER_EC                            = 62,
    TDF_WATER_TURBIDITY                     = 63,
    TDF_WATER_FLOW_RATE                     = 64,
    TDF_WATER_TOTAL_VOLUME                  = 65,
    TDF_WATER_PRESSURE                      = 66,
    TDF_WATER_DEPTH                         = 67,
    TDF_AD595_TEMP                          = 68,
    TDF_DS1626_TEMP                         = 69,
    TDF_DS1626_TEMP_N                       = 70,
    TDF_AIR_TEMP                            = 71,
    TDF_AIR_HUMIDITY                        = 72,
    TDF_WIND_SPEED                          = 73,
    TDF_WIND_DIRECTION                      = 74,
    TDF_RAINFALL                            = 75,
    TDF_LEAF_WETNESS                        = 76,
    TDF_LIGHT_INTENSITY_PAR                 = 77,
    TDF_MULTI_SPECTRAL_ARRAY                = 78,
    TDF_PIR_COUNT                           = 79,
    TDF_PACKET_ID                           = 80,
    TDF_GPS_LAT_LONG                        = 81,
    TDF_GPS_NORTHING_EASTING                = 82,
    TDF_GPS_HEIGHT_ABOVE_ELLIPSOID          = 83,
    TDF_GPS_TIME_OF_WEEK                    = 84,
    TDF_GPS_WEEK                            = 85,
    TDF_GPS_NUM_SATELLITES                  = 86,
    TDF_GPS_HORZ_ACCURACY                   = 87,
    TDF_GPS_VERT_ACCURACY                   = 88,
    TDF_GPS_DOP                             = 89,
    TDF_GPS_SIGNAL_STRENGTH                 = 90,
    TDF_GPS_SIGNAL_QUALITY                  = 91,
    TDF_GPS_HMSL                            = 92,
    TDF_LOAD_CELL                           = 93,
    TDF_TAG_READER                          = 94,
    TDF_ZAP_DURATION                        = 95,
    TDF_ZAP_INTENSITY                       = 96,
    TDF_ZAP_COUNT                           = 97,
    TDF_AUDIO_COUNT                         = 98,
    TDF_FENCE_LINE                          = 99,
    TDF_VF_FLAGS                            = 100,
    TDF_FLOOR_NUM                           = 101,
    TDF_PEDOMETRY_COUNT                     = 102,
    TDF_DATA_TRANSFER                       = 103,
    TDF_IR_RANGE                            = 104,
    TDF_TEMP_STRING                         = 105,
    TDF_BEACON                              = 106,
    TDF_SOLAR_CHARGE                        = 107,
    TDF_POWER_SENSORHUB_UPS_SOURCE          = 108,
    TDF_POWER_SENSORHUB_UPS_BATTERY_VOLTS   = 109,
    TDF_SENSORHUB_NUM_CMD                   = 110,
    TDF_SENSORHUB_NUM_MUTEX_FAIL            = 111,
    TDF_SENSORHUB_NUM_SERIAL_RETRY          = 112,
    TDF_SENSORHUB_NUM_SERIAL_FAIL           = 113,
    TDF_SENSORHUB_NUM_RPC_FAIL              = 114,
    TDF_TEMP_SENSIRION_SHT15                = 115,
    TDF_TEMP_ROTRONIC_HYGROCLIP2            = 116,
    TDF_TEMP_KFS140_CUSTOM                  = 117,
    TDF_TEMP_MAXIM_IBUTTON                  = 118,
    TDF_HUMID_SENSIRION_SHT15               = 119,
    TDF_HUMID_ROTRONIC_HYGROCLIP2           = 120,
    TDF_HUMID_KFS140_CUSTOM                 = 121,
    TDF_HUMID_MAXIM_IBUTTON                 = 122,
    TDF_SOIL_DECAGON_EC5                    = 123,
    TDF_SOIL_DECAGON_EC20                   = 124,
    TDF_SOIL_DECAGON_10HS                   = 125,
    TDF_SOIL_DECAGON_MPS1                   = 126,
    TDF_SOIL_DECAGON_ECTM                   = 127,
    TDF_SOIL_DECAGON_5TE                    = 128,
    TDF_LIGHT_APOGEE_SQ110                  = 129,
    TDF_LIGHT_APOGEE_SP110                  = 130,
    TDF_LIGHT_MIDDLETON_SK01DP2             = 131,
    TDF_LIGHT_MIDDLETON_SK08                = 132,
    TDF_LEAFWET_DECAGON_LWS                 = 133,
    TDF_WIND_DIRECTION_METONE_034B          = 134,
    TDF_WIND_SPEED_METONE_034B              = 135,
    TDF_WIND_DIRECTION_VAISALA_WXT520       = 136,
    TDF_WIND_SPEED_VAISALA_WXT520           = 137,
    TDF_PTU_VAISALA_WXT520                  = 138,
    TDF_RAIN_VAISALA_WXT520                 = 139,
    TDF_HAIL_VAISALA_WXT520                 = 140,
    TDF_LPL_SLEEP_STATS                     = 141,
    TDF_WIND_DIRECTION_METONE_034B_V2       = 142,
    TDF_WIND_SPEED_METONE_034B_V2           = 143,
    TDF_PRECIP_RESET_VAISALA_WXT520         = 144,
    TDF_SOIL_MOISTURE_SPADE                 = 145,
    TDF_SOIL_TEMP_SPADE                     = 146,
    TDF_NETWORK_AT86RF212_STATS             = 147,
    TDF_NETWORK_XMAC_STATS                  = 148,
    TDF_NETWORK_DISPATCHER_STATS            = 149,
    TDF_NETWORK_FLOODING_STATS              = 150,
    TDF_NETWORK_CTP_STATS                   = 151,
    TDF_NETWORK_CTP_STATE                   = 152,
    TDF_TEMP_MICROCHIP_MCP9800              = 153,
    TDF_POWER_SOURCE                        = 154,
    TDF_BATTERY_VOLTAGE_UPS                 = 155,
    TDF_BATTERY_CHARGE_STATUS               = 156,
    TDF_SYSTEM_INFO                         = 157,
    TDF_SYSTEM_APP_NAME                     = 158,
    TDF_SYSTEM_APP_VERSION                  = 159,
    TDF_SOIL_DECAGON_5TE_V2                 = 160,
    TDF_TEMP_HUMID_SENSIRION_SHT15          = 161,
    TDF_TEMP_HUMID_SENSIRION_SHT15_HIGH     = 162,
    TDF_TEMP_APOGEE_ST100                   = 163,
    TDF_TEMP_APOGEE_ST100_HIGH              = 164,
    TDF_TEMP_APOGEE_ST100_GROUND            = 165,
    TDF_TEMP_APOGEE_ST100_SOIL              = 166,
    TDF_DENDROMETER_ICT_DR26                = 167,
    TDF_DENDROMETER_ICT_DR26_2              = 168,
    TDF_DENDROMETER_ICT_DR26_3              = 169,
    TDF_DENDROMETER_ICT_DR26_4              = 170,
    TDF_DENDROMETER_ICT_DR26_5              = 171,
    TDF_DENDROMETER_ICT_DR26_6              = 172,
    TDF_TENSIOMETER_ICT_GT3_15              = 173,
    TDF_TEMP_VAISALA_HMP60                  = 174,
    TDF_HUMID_VAISALA_HMP60                 = 175,
    TDF_CO2_DYNAMENT_1000PPM                = 176,
    TDF_CO2_DYNAMENT_1000PPM_HIGH           = 177,
    TDF_LEAFWET_DECAGON_LWS_5V              = 178,
    TDF_LEAFWET_DECAGON_LWS_5V_HIGH         = 179,
    TDF_LEAFWET_DECAGON_LWS_5V_GROUND       = 180,
    TDF_LIGHT_APOGEE_SQ110_HIGH             = 181,
    TDF_LIGHT_APOGEE_SP110_HIGH             = 182,
    TDF_FOG_OPTICAL_MINIOFS                 = 183,
    TDF_FOG_OPTICAL_MINIOFS_HIGH            = 184,
    TDF_CLOUD_OPTICAL_PCCD                  = 185,
    TDF_WIND_DIRECTION_VAISALA_WXT520_HIGH  = 186,
    TDF_WIND_SPEED_VAISALA_WXT520_HIGH      = 187,
    TDF_PTU_VAISALA_WXT520_HIGH             = 188,
    TDF_RAIN_VAISALA_WXT520_HIGH            = 189,
    TDF_HAIL_VAISALA_WXT520_HIGH            = 190,
    TDF_CO2_VAISALA_GMP343                  = 191,
    TDF_RAIN_ICT_SSRG1                      = 192,
    TDF_SAPFLOW_ICT_SFM1_1                  = 193,
    TDF_SAPFLOW_ICT_SFM1_2                  = 194,
    TDF_SAPFLOW_ICT_SFM1_3                  = 195,
    TDF_WIND_DIRECTION_VAISALA_WMT52        = 196,
    TDF_WIND_SPEED_VAISALA_WMT52            = 197,
    TDF_WIND_DIRECTION_VAISALA_WMT52_HIGH   = 198,
    TDF_WIND_SPEED_VAISALA_WMT52_HIGH       = 199,
    TDF_SOIL_DECAGON_5TM                    = 200,
    TDF_TEMP_APOGEE_SI121                   = 201,
    TDF_TEMP_EXERGEN_IRTC07_K               = 202,
    TDF_TEMP_EXERGEN_IRTC03_K               = 203,
    TDF_LIGHT_DELTAT_TLS                    = 204,
    TDF_LIGHT_DELTAT_TLS_2                  = 205,
    TDF_TEMP_MELEXIS_MLX90614ESF_ACF        = 206,
    TDF_TEMP_T_THERMOCOUPLE                 = 207,
    TDF_TEMP_T_THERMOCOUPLE_2               = 208,
    TDF_TEMP_T_THERMOCOUPLE_3               = 209,
    TDF_TEMP_T_THERMOCOUPLE_4               = 210,
    TDF_BMP085_RAW                          = 211,
    TDF_BMP085_COMPENSATED                  = 212,
    TDF_PACP_POWER                          = 213,
    TDF_GAS_METHANE                         = 214,
    TDF_GAS_CO2                             = 215,
    TDF_GAS_HYDROGEN                        = 216,
    TDF_EARTAG_POWER                        = 217,
    TDF_LSM303                              = 218,
    TDF_TEMP_HUMID_SHT25_RAW                = 219,
    TDF_TEMP_HUMID_SHT25_COMPENSATED        = 220,
    TDF_LIGHT_AVAGO_CR999                   = 221,
    TDF_REED_SWITCH                         = 222,
    TDF_PACP_AUDIO_RAW                      = 223,
    TDF_PACP_AUDIO_SILENCE                  = 224,
    TDF_ACC_LSM303                          = 225,
    TDF_MAG_LSM303                          = 226,
    TDF_RADIO_STATS                         = 227,
    TDF_LAMBING_STATS                       = 228,
    TDF_GPS_POSLLH                          = 229,
    TDF_GPS_TIMEGPS                         = 230,
    TDF_GPS_NAVSOL                          = 231,
    TDF_SD_PAGE_NUM                         = 232,
    TDF_WEIGHT_BRIDGE                       = 233,
    TDF_OPAL_ONBOARD_STATS                  = 234,
    TDF_PACP_PRECISION_POWER                = 235,
    TDF_BATTERY_SOLAR                       = 236,
    TDF_FLASH_DATA                          = 237,
    TDF_FLASH_PAGE                          = 238,
    TDF_GPS_POSITION                        = 239,
    TDF_PACP_RESET_COUNT                    = 240,
    TDF_UPTIME                              = 241,
    TDF_FLASH_DATA_LOCATION                 = 242,
    TDF_GAS_IR15TT                          = 243,
    TDF_MPU9150                             = 244,
    TDF_PACP_WIND_VAISALA_WXT520            = 245,
    TDF_PACP_PTH_VAISALA_WXT520             = 246,
    TDF_PACP_PRECIP_VAISALA_WXT520          = 247,
    TDF_PACP_SUPER_VAISALA_WXT520           = 248,
    TDF_PACP_RESET_REASON                   = 249,
    TDF_GPS_STATUS                          = 250,
    TDF_PACP_POWER_STATUS                   = 251,
    TDF_BUILD_TIMESTAMP                     = 252,
    TDF_PAGE_DOWNLOAD_STATS                 = 253,
    TDF_MPU9150_ALL                         = 254,
    TDF_LPS331_ALL                          = 255,
    TDF_AD7689                              = 256,
    TDF_AD7689_SINGLE                       = 257,
    TDF_GATEWAY_POSITION                    = 258,
    TDF_PACP_AUDIO_DATA                     = 259,
    TDF_PACP_AUDIO_DIFF                     = 260,
    TDF_LSM303DLHC_ACC                      = 261,
    TDF_LSM303DLHC_MAG                      = 262,
    TDF_LSM303DLHC_ACC_INTERRUPT            = 263,
    TDF_RADIO_CONTACT_LOG                   = 264,
    TDF_DEBUG_TEXT                          = 265,
    TDF_APP_INFO                            = 266,
    TDF_KERNEL_VERSION                      = 267,
    TDF_SOFTWARE_VERSION                    = 268,
    TDF_BASESTATION_INFO                    = 269,
    TDF_STREET_ADDRESS                      = 270,
    TDF_GAS_TGS821                          = 271,
    TDF_PAGE_WRITE_STATS                    = 272,
    TDF_PACP_AUDIO                          = 273,
    TDF_ANNOTATION                          = 274,
    TDF_RSSI_ROUNDTRIP                      = 275,
    TDF_MP3H6115A                           = 276,
    TDF_TMP112                              = 277,
    TDF_GPS_RECEIVER_STATUS                 = 278,
    TDF_GPS_TTFF                            = 279,
    TDF_PACP_1K24_POWER                     = 280,
    TDF_GPS                                 = 281,
    TDF_MODEM_TRAFFIC_STATS                 = 282,
    TDF_DEBUG_TEXT_SHORT                    = 283,
    TDF_RADIO_PACKET_STATS                  = 284,
    TDF_GPS_HARDWARE_VERSION                = 285,
    TDF_DISK_USAGE_STATS                    = 286,
    TDF_SPECIES                             = 287,
    TDF_GATEWAY_SCHEDULE                    = 288,
    TDF_GAS_TGS832                          = 289,
    TDF_AD7718_CHANNEL                      = 290,
    TDF_ANDROID_BATTERY                     = 291,
    TDF_MOBILE_SIGNAL                       = 292,
    TDF_IAQCORE_VOC                         = 293,
    TDF_KHS200_H2                           = 294,
    TDF_RADIO_CONTACT                       = 295,
    TDF_IBEACON                             = 296,
    TDF_RADIATION                           = 297,
    TDF_IR15TT_CALIBRATED                   = 298,
    TDF_RADIATION_DELTA                     = 299,
    TDF_RADIATION_SIEVERT                   = 300,
    TDF_GP2Y_RANGE                          = 301,
    TDF_RADIATION_CPS                       = 302,
    TDF_SD_PAGES_PER_MIN                    = 303,
    TDF_MISSED_DATA                         = 304,
    TDF_EXTERNAL_12V_BATTERY                = 305,
    TDF_STATUS                              = 306,
    TDF_GATEWAY_STATUS                      = 307,
    TDF_DECAGON_5TM_POTTING_SOIL            = 308,
    TDF_IBEACON_MAJOR_MINOR                 = 309,
    TDF_HIH7XXX_SENSOR                      = 310,
    TDF_PACP_2_POWER_STATUS                 = 311,
    TDF_BMP280_RAW                          = 312,
    TDF_MPU9250_ACC                         = 313,
    TDF_SENSORTAG_TEMP                      = 314,
    TDF_SENSORTAG_HUMID                     = 315,
    TDF_SENSORTAG_LIGHT                     = 316,
    TDF_HDC1000_RAW                         = 317,
    TDF_SENSORTAG_IBEACON_BASIC             = 318,
    TDF_PIR                                 = 319,
    TDF_DAD_POWER                           = 320,
    TDF_BMP280                              = 321,
    TDF_PIR_EVENT                           = 322,
    TDF_MPU9250_ACC_RAW                     = 323,
    TDF_MPU9X50_2G_500DPS                   = 324,
    TDF_DETER_SOUND                         = 325,
    TDF_TIME_BREAK                          = 326,
    TDF_LTC2942                             = 327,
    TDF_DETER_LIGHT                         = 328,
    TDF_PACP2_ADC                           = 329,
    TDF_NW_NODEID_0412                      = 330,
    TDF_NW_NODEID_0808                      = 331,
    TDF_NW_NODEID_0816                      = 332,
    TDF_OYSTAG_ENVIRONMENTAL                = 333,
    TDF_OYSTAG_DIAGNOSTIC                   = 334,
    TDF_OYSTAG_PHYSIOLOGICAL                = 335,
    TDF_OYSTAG_TELEMETRY_DIAGNOSTIC         = 336,
    TDF_OYSTAG_TELEMETRY_CONNECTED_IDS      = 337,
    TDF_OYSTAG_TELEMETRY_LORA               = 338,
    TDF_LORA_ROUND_TRIP                     = 339,
    TDF_LORA_RECV                           = 340,
    TDF_LOCI_POWER                          = 341,
    TDF_LPS22HB_ALL                         = 342,
    TDF_SHT31DIS_ALL                        = 343,
    TDF_MPU9250_WOM_INTERRUPT               = 344,
    TDF_MPU9250_MAG                         = 345,
    TDF_OPT3001                             = 346,
    TDF_VPDAD_JUMBO                         = 347,
    TDF_RELAY_JUMBO                         = 348,
    TDF_NW_NODEID_ASSIGN                    = 349,
    TDF_VPDAD_TEST                          = 350,
    TDF_LORAWAN_LINK                        = 351,
    TDF_MS5837                              = 352,
    TDF_PIR_TRIGGER                         = 353,
    TDF_GPS_LLH                             = 354,
    TDF_GPS_COMPACT                         = 355,
    TDF_CONDUIT_POWER                       = 356,
    TDF_OYSTAG_HEART_STATS                  = 357,
    TDF_SALT_WATER_SWITCH                   = 358,
    TDF_GPS_LOCK_STATS                      = 359,
    TDF_TURTAG_DIAG                         = 360,
    TDF_TURTAG_SOLAR                        = 361,
    TDF_TURTAG_GPS_SLIM                     = 362,
    TDF_TURTAG_DIVE                         = 363,
    TDF_TURTAG_SURFACE                      = 364,
    TDF_GPS_FAIL_STATS                      = 365,
    TDF_TURTAG_SOLAR_FLASH                  = 366,
    TDF_TURTAG_DEPTH                        = 367,
    TDF_CVM_INFO                            = 368,
    TDF_MS5837_COMPACT                      = 369,
    TDF_CERES_SOLAR                         = 370,
    TDF_LORA_RSSI                           = 371,
    TDF_TURTAG_BASIC                        = 372,
    TDF_DEVICE_STATUS_BLE                   = 373,
    TDF_EZO_SPARE                           = 374,
    TDF_LORA_STATS                          = 375,
    TDF_PACP2_PINGER_VOLTAGE                = 376,
    TDF_VPDAD_JUMBO_SMALL                   = 377,
    TDF_EZO_ORP                             = 378,
    TDF_EZO_DO                              = 379,
    TDF_EZO_PH                              = 380,
    TDF_EZO_RTD                             = 381,
    TDF_EZO_EC                              = 382,
    TDF_EZO_UK                              = 383,
    TDF_CVM_CLASSIFICATION                  = 384,
    TDF_UNIX_TIME                           = 385,
    TDF_WATCHDOG_INFO                       = 386,
    TDF_GPS_PVT                             = 387,
    TDF_GPS_LOC_COUNT                       = 388,
    TDF_VPDAD_SENTINEL                      = 389,
    TDF_3G_STATS                            = 390,
    TDF_FERAL_JUMBO                         = 391,
    TDF_MPU9250_ECOMPASS_4G                 = 392,
    TDF_UBX_NAV_SAT                         = 393,
    TDF_GPS_FERAL_JUMBO                     = 394,
    TDF_GPS_LATLON                          = 395,
    TDF_RELAY_MDOT_PACKET_STATS             = 396,
    TDF_TMP116                              = 397,
    TDF_ACC_STATS                           = 398,
    TDF_RELAY_LOCI_POWER                    = 399,
    TDF_GPS_COMPACT_RELAY                   = 400,
    TDF_RELAY_UPTIME                        = 401,
    TDF_RELAY_RX_COUNT                      = 402,
    TDF_RELAY_RADIO_STATS                   = 403,
    TDF_RELAY_RESET_STATS                   = 404,
    TDF_CLASSIFICATION                      = 405,
    TDF_LORA_DEBUG_PKT                      = 406,
    TDF_TELEMETRY_DIAGNOSTIC                = 407,
    TDF_CONTACT_SLOT                        = 408,
    TDF_CC2650_CONTACT_LOG                  = 409,
    TDF_WATER_CONDUCTIVITY                  = 410,
    TDF_BLEAT_ASSET                         = 411,
    TDF_BLEAT_ASSET_LL                      = 412,
    TDF_BLEAT_ASSET_LL_COMPACT              = 413,
    TDF_SCHEDULER_STATE_SET                 = 414,
    TDF_SCHEDULER_STATE_CLEAR               = 415,
    TDF_SCHEDULER_STATE_TIME                = 416,
    TDF_DIAGNOSTIC_BASE_TIME                = 417,
    TDF_DIAGNOSTIC_NODE_TIME                = 418,
    TDF_PIEZO                               = 419,
    TDF_GPS_MEASUREMENT                     = 420,
    TDF_STS31                               = 421,
    TDF_ACC_XYZ_SIGNED                      = 422,
    TDF_CERES_JUMBO                         = 423,
    TDF_BATTERY_STATS                       = 424,
    TDF_BLEATCON_LAT_LNG                    = 425,
    TDF_RELAY_ID                            = 426,
    TDF_BLEACON_JUMBO                       = 427,
    TDF_BLEAT_ASSET_JUMBO                   = 428,
    TDF_LOST_DATA                           = 429,
    TDF_GPS_LOCK_STATS_ACC                  = 430,
    TDF_ACCEL_VARIANCE                      = 431,
    TDF_STATS_SUMMARY                       = 432,
    TDF_SYSTEM_ANNOUNCE                     = 433,
    TDF_DTREE_CLASS                         = 434,
    TDF_DTREE_CLASS_SUM                     = 435,
    TDF_DTREE_GRAD_VAR                      = 436,
    TDF_WATER                               = 437,
    TDF_PIN_STATUS                          = 438,
    TDF_CSIROTRACK_APP_SETTINGS             = 439,
    TDF_INFRINGEMENT_NOTIFICATION           = 440,
    TDF_WATCHDOG_INFO_SMALL                 = 441,
    TDF_CERES_JUMBO_SMALL                   = 442,
    TDF_CERES_GS_LOCATION                   = 443,
    TDF_ENV_TEMPERATURE                     = 444,
    TDF_ENV_TMP_HUM_PRE                     = 445,
    TDF_CSIROTRACK_HOURLY                   = 446,
    TDF_CSIROTRACK_ENV                      = 447,
    TDF_MPU9250_RAW                         = 448,
    TDF_GPS_CSIROTRACK                      = 449,
    TDF_HDC1000                             = 450,
    TDF_BITE_COUNT                          = 451,
    TDF_EGRAZOR_CLAS_SUM                    = 452,
    TDF_ACC_ROLLPITCH                       = 453,
    TDF_BLEAT_SETTINGS                      = 454,
    TDF_BLEAT_STATUS                        = 455,
    TDF_BLEACON_STATUS_DATA                 = 456,
    TDF_CT_GLOBALSTAR_ECHO                  = 457,
    TDF_BLEAT_LOCATION                      = 458,
    TDF_ACC_XYZ_2G                          = 459,
    TDF_ACC_XYZ_4G                          = 460,
    TDF_ACC_XYZ_8G                          = 461,
    TDF_ACC_XYZ_16G                         = 462,
    TDF_GENERIC_EVENT                       = 463,
    TDF_SCHEDULER_ERROR                     = 464,
    TDF_SI1133_DATA                         = 465,
    TDF_EGRAZOR_6CLAS_SUM                   = 466,
    TDF_BLEACON_STATS                       = 467,
    TDF_BATTERY_PROCESSED                   = 468,
    TDF_TRANSPORT_MODE                      = 469,
    TDF_FRAUD_DETECTION_STATE               = 470,
    TDF_LSM6DSL                             = 471,
    TDF_3D_POSE                             = 472,
    TDF_MAG_XYZ_SIGNED                      = 473,
    TDF_HEADING                             = 474,
    TDF_RANGE_CM                            = 475,
} eTdfIds_t;

/* External Variables ---------------------------------------*/

extern const uint8_t pucTdfStructLengths[476];

// clang-format on
#endif /* __CORE_CSIRO_LIBRARIES_TDF_AUTO */
