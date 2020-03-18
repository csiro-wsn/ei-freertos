# EI-FREERTOS

## Overview

Embedded systems framework utilising FreeRTOS for distributed, low-power sensing applications.
Developed by the CSIRO Data61 'Embedded Intelligence' team

## Software Setup

Run the repo_setup.sh script

## Hardware Setup

Before using Particle devices with this codebase, they must be completely erased via jlink.
To do so, in any application directory (apps/unifiedbase for example):
```
make TARGET=argon jlink
```
Once the J-Link prompt appears:
```
erase
```
Once erasing has finished, Ctrl+C to exit J-Link
