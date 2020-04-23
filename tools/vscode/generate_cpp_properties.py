#!/usr/bin/env python3

"""
generate_cpp_properties.py

Configuration file generator for the pacp-freertos repository using vscode.
  - Based on an application and platform, sets up include paths so
    intellisense works correctly
"""
__author__ = "Jordan Yates"

import argparse
import json
import os

properties_template = {
    "configurations": [
        {
            "name": "Linux",
            "cStandard": "c99",
            "compilerPath": "",
            "intelliSenseMode": "clang-x64",
            "includePath": [],
            "defines": []
        }
    ],
    "version": 4
}

platform_mappings = {
    "thunderboard2": {
        "architecture": "EFR32",
        "family": "EFR32MG12P",
        "cpu": "EFR32MG12P332F1024GL125",
        "defines": []
    },
    "ceresig": {
        "architecture": "EFR32",
        "family": "EFR32BG13P",
        "cpu": "EFR32BG13P732F512GM48",
        "defines": []
    },
    "nrf52840dk": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    },
    "minew": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52832",
        "softdevice": "s132",
        "defines": ["NRF52832_XXAA"]
    },
    "nrf52832dk": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52832",
        "softdevice": "s140",
        "defines": ["NRF52832_XXAA"]
    },
    "bleatag": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    },
    "bleacon": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    },
    "ceresat": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    },
    "ceresat_old": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    },
    "linux": {
        "architecture": "linux",
        "family": "linux",
        "cpu": "linux",
        "defines": []
    },
    "argon": {
        "architecture": "nrf52",
        "family": "nrf52",
        "cpu": "nrf52840",
        "softdevice": "s140",
        "defines": ["NRF52840_XXAA"]
    }
}

includes = {
    "constant": [
        # Core CSIRO Include Directories
        "${workspaceFolder}/core_csiro/arch/common",
        "${workspaceFolder}/core_csiro/arch/common/bluetooth/inc",
        "${workspaceFolder}/core_csiro/arch/common/FreeRTOS/inc",
        "${workspaceFolder}/core_csiro/arch/common/interface/inc",
        "${workspaceFolder}/core_csiro/arch/common/nvm/inc",
        "${workspaceFolder}/core_csiro/batteries/inc",
        "${workspaceFolder}/core_csiro/comms/inc",
        "${workspaceFolder}/core_csiro/common/inc",
        "${workspaceFolder}/core_csiro/interfaces/inc",
        "${workspaceFolder}/core_csiro/libraries/inc",
        "${workspaceFolder}/core_csiro/loggers/inc",
        "${workspaceFolder}/core_csiro/peripherals/memory/inc",
        "${workspaceFolder}/core_csiro/peripherals/sensors/inc",
        "${workspaceFolder}/core_csiro/peripherals/utility/inc",
        "${workspaceFolder}/core_csiro/platform/common/inc",
        "${workspaceFolder}/core_csiro/rpc/inc",
        "${workspaceFolder}/core_csiro/scheduler/activities/inc",
        "${workspaceFolder}/core_csiro/scheduler/core/inc",
        "${workspaceFolder}/core_csiro/scheduler/interfaces/inc",
        "${workspaceFolder}/core_csiro/tasks/core/inc",
        "${workspaceFolder}/core_csiro/tasks/peripheral_validation/inc",
        # Core External Include Directories
        "${workspaceFolder}/core_external/CMSIS_5/CMSIS/Core/Include",
        "${workspaceFolder}/core_external/CMSIS_5/CMSIS/DSP/Include",
        "${workspaceFolder}/core_external/FreeRTOS/Source/include",
        "${workspaceFolder}/core_external/FreeRTOS/Source/portable/GCC/ARM_CM4F",
        "${workspaceFolder}/core_external/tiny_printf/inc"
    ],
    "formatted": [
        "${{workspaceFolder}}/core_csiro/platform/{target}/inc",
        "${{workspaceFolder}}/core_csiro/arch/{architecture}/bluetooth/inc",
        "${{workspaceFolder}}/core_csiro/arch/{architecture}/cpu/{cpu}/inc",
        "${{workspaceFolder}}/core_csiro/arch/{architecture}/interface/inc",
    ],
    "unit_test": [
        "${workspaceFolder}/unit_tests/mocks/bluetooth_mocks/inc",
        "${workspaceFolder}/unit_tests/mocks/cortex_mocks/inc",
        "${workspaceFolder}/unit_tests/mocks/library_mocks/inc",
        "${workspaceFolder}/unit_tests/mocks/mbed_mocks/inc",
        "${workspaceFolder}/unit_tests/mocks/EFR32_mocks/inc",
        "${workspaceFolder}/unit_tests/mocks/FreeRTOS_mocks/inc"
    ],
    "EFR32": {
        "constant": [
            "${workspaceFolder}/core_external/gecko_sdk/protocol/bluetooth/ble_stack/inc/common",
            "${workspaceFolder}/core_external/gecko_sdk/protocol/bluetooth/ble_stack/inc/soc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/bootloader/api",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/common/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/dmadrv/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/gpiointerrupt/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/nvm3/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/spidrv/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emdrv/uartdrv/inc",
            "${workspaceFolder}/core_external/gecko_sdk/platform/emlib/inc",
            "${workspaceFolder}/core_external/gecko_sdk/util/third_party/mbedtls/include"
        ],
        "formatted": [
            "${{workspaceFolder}}/core_external/gecko_sdk/platform/Device/SiliconLabs/{family}/Include"
        ]
    },
    "nrf52": {
        "constant": [
            "${workspaceFolder}/core_csiro/platform/nrf52840dk/inc",
            "${workspaceFolder}/core_external/nrf52_sdk/components/libraries/fds",
            "${workspaceFolder}/core_external/nrf52_sdk/components/libraries/util",
            "${workspaceFolder}/core_external/nrf52_sdk/components/libraries/experimental_section_vars",
            "${workspaceFolder}/core_external/nrf52_sdk/components/softdevice/s140/headers",
            "${workspaceFolder}/core_external/nrf52_sdk/components/ble/common",
            "${workspaceFolder}/core_external/nrf52_sdk/components/softdevice/common",
            "${workspaceFolder}/core_external/nrf52_sdk/integration/nrfx",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx/drivers",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx/drivers/include",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx/drivers/src/prs",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx/hal",
            "${workspaceFolder}/core_external/nrf52_sdk/modules/nrfx/mdk"
        ],
        "formatted": [
            "${{workspaceFolder}}/core_external/nrf52_sdk/components/softdevice/{softdevice}/headers/nrf52"
        ]
    },
    "linux": {
        "constant": [],
        "formatted": []
    }
}

defines = {
    "EFR32": {
        "constant": ["MBEDTLS_AES_ALT", "MBEDTLS_CONFIG_FILE=\"<mbedtls_config.h>\""],
        "formatted": ["{family}", "{cpu}"]
    },
    "nrf52": {
        "constant": ["SOFTDEVICE_PRESENT", "MBEDTLS_CONFIG_FILE=\"<mbedtls_config.h>\""],
        "formatted": ["{architecture}", "{cpu}"]
    },
    "linux": {
        "constant": [],
        "formatted": []
    }
}

if __name__ == "__main__":
    # Windows example compilerPath
    # "\"C:\\Program Files (x86)\\GNU Tools ARM Embedded\\8 2018-q4-major\\bin\\arm-none-eabi-gcc.exe\""

    parser = argparse.ArgumentParser(description='Generate c_cpp_properties.json file for vscode')
    parser.add_argument('--app', dest='app', required=True, help='Application name for includes')
    parser.add_argument('--dest', dest='dest', required=True, help='Location to write c_cpp_properties.json')
    parser.add_argument('--target', dest='target', required=True, help='Target name', choices=list(platform_mappings.keys()))
    parser.add_argument('--compiler', dest='compiler_path', required=True, help='Path to arm-non-eabi-gcc')
    parser.add_argument('--app_dir', dest='app_dir', required=True, help='Path to application directory')
    args = parser.parse_args()

    mappings = platform_mappings[args.target]
    mappings["app"] = args.app
    mappings["target"] = args.target

    # Add the -mcpu options so that the compiler knows the registers etc of the target
    properties_template["configurations"][0]["compilerPath"] = args.compiler_path + " -mcpu=cortex-m4"

    properties_template["configurations"][0]["includePath"].extend([os.path.join(args.app_dir, 'inc')])
    properties_template["configurations"][0]["includePath"].extend([path.format(**mappings) for path in includes[mappings["architecture"]]["formatted"]])
    properties_template["configurations"][0]["includePath"].extend(includes[mappings["architecture"]]["constant"])
    properties_template["configurations"][0]["includePath"].extend([path.format(**mappings) for path in includes["formatted"]])
    properties_template["configurations"][0]["includePath"].extend(includes["constant"])
    properties_template["configurations"][0]["includePath"].extend(includes["unit_test"])

    properties_template["configurations"][0]["defines"].extend([path.format(**mappings) for path in defines[mappings["architecture"]]["formatted"]])
    properties_template["configurations"][0]["defines"].extend(defines[mappings["architecture"]]["constant"])
    properties_template["configurations"][0]["defines"].extend(mappings["defines"])

    output_file = os.path.join(args.dest, 'c_cpp_properties.json')
    with open(output_file, 'w') as f:
        json.dump(properties_template, f, indent='\t')
        f.write(os.linesep)
