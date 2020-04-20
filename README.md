# ei-freertos

## Overview

Embedded systems framework utilising FreeRTOS for distributed, low-power sensing applications.

Developed by the CSIRO Data61 Embedded Intelligence team.

## Repo Setup

Run the repo_setup.sh script. Follow its instructions.

If you prefer lists of instructions, you can follow the toolchain setup instructions at the bottom of this README.

Alternatively check out the repo setup tutorial for a walkthrough of the tutorial setup.

## Tutorials

There are a number of tutorials covering topics ranging from repository setup to logging data over bluetooth.

There can be found here: https://www.youtube.com/playlist?list=PLY5XmR6T00-DvnNMEfenThWIh7v3epEmV.

## Toolchain Setup Instructions

* Install VScode (optional, but highly recommended. We have scripts that autogenerate the intellisense paths for vscode so it will make development far easier).
* Install git 
    
        sudo apt install git
* Install gcc-arm-none-eabi. This is done differently for each OS, so pick your poison below.
    * How to Install arm-none-eabi-gcc for Linux:

            # If you have already installed gcc-arm-none-eabi and you're having problems, uninstall it with:
            sudo apt-get remove gcc-arm-none-eabi
            # Then install the most up-to-date version with this command:
            sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
            sudo apt-get update
            sudo apt-get install gcc-arm-embedded
    * How to Install arm-none-eabi-gcc for OSX

            brew tap ArmMbed/homebrew-formulae
            brew install arm-none-eabi-gcc
* Clone down required repositories
    * Make code directory and cd into it:

            mkdir code
            cd code
    * Clone down the repo:
    
            git clone https://github.com/csiro-wsn/ei-freertos.git
* Attempt to run setup script. If we are missing anything it will let us know:

        ./repo_setup.sh
* Download Segger's JLink
    * Install / add to the path.
    * The install script checks to see if the binary "JLinkExe" is on the path. Ensure that it is.
* Run setup script

        ./repo_setup.sh
* Install pip to get packages for python. Good time to mention that all our tools use python3, so if you have python3 and pip3 that's good. If you have python2 and pip2, then maybe your paths will get confusing and bad things will happen.

        sudo apt install python3-pip
* Run setup script again

        ./repo_setup.sh
* Add the following lines to your bash_profile:
    * If you're on Linux

            CODE=/home/{your_username_here}/code
            
            export PATH=$CODE/ei-freertos/tools:$PATH
            
            export PYTHONPATH=$PYTHONPATH:$CODE/ei-freertos/pyclasses
    * If you're on OSX:

            CODE=/Users/{your_username_here}/code
            
            export PATH=$CODE/ei-freertos/tools:$PATH
            
            export PYTHONPATH=$PYTHONPATH:$CODE/ei-freertos/pyclasses
* So your terminal actually updates, either close and re-open it, or type:

        source ~/.bash_profile
* Run a number of make commands to show how they work and ensure that they DO work.
    * make vscode TARGET=bleatag
    * make flashdbg TARGET=bleatag -j
    * make test_python
    * make gdb TARGET=bleatag

These are all useful make commands. Have a look at their descriptions by running 'make help' if you want to know more.
