#!/bin/bash

CURRENT_PATH=$PWD
CURRENT_DIR=`basename $CURRENT_PATH`
OS=`uname`

# Colours
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Check whether JLink is installed already. If it's not, politely direct the user to install it.
if ! [ -x "$(command -v JLinkExe)" ]; then
    echo ""
    echo "You don't have JLink currently installed." >&2
    echo "Install it from: \"https://www.segger.com/downloads/jlink/\" and add this line to your bash profile:"
    echo "export PATH=\"\$PATH:/Applications/SEGGER/JLink\""
    echo ""
    exit 1
fi

# Validate that python3 is installed, suggest locations to install from
if ! [ -x "$(command -v python3)" ]; then
    echo ""
    echo "You don't have python3 currently installed." >&2
    echo "You can install it via 'brew install python3' or another source (Anaconda etc)"
    echo ""
    exit 1
fi

if ! [ -x "$(command -v pip3)" ]; then
    echo ""
    echo "You don't have python3 pip currently installed." >&2
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "pip3 can be installed with 'brew install python3'"
    else
        echo "pip3 can be installed with 'sudo apt install python3-pip'"
    fi
    
    echo "If an alternate source is used, please add location of pip3 to \$PATH"
    echo ""
    exit 1
fi

# Automatically try to install required tools
# Wrap install in loop for early exit
while [ 1 ]; do
    # MacOSX, install dependencies using brew
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # Validate that brew is installed
        if ! [ -x "$(command -v brew)" ]; then
            # Ask user if they wish to install brew
            echo ""
            echo "Brew is not currently installed..."
            echo "Brew is required to automatically install required dependencies"
            echo ""
            read -p "Do you wish to install it? " -n 1 -r
            echo    # (optional) move to a new line
            if [[ $REPLY =~ ^[Yy]$ ]]
            then
                /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
            else
                "Brew was not installed, skipping automatic installation"
                break
            fi
        fi
        echo "Install required packages with brew"
        brew install libtool automake lcov clang-format graphviz
        break
    # Linux, install with apt-get
    elif [[ "$OSTYPE" == "linux-gnu" ]]; then
        echo "Install required packages with apt-get"
        sudo apt-get install libtool-bin automake lcov binutils clang-format
        break
    # FreeBSD, install using ??
    elif [[ "$OSTYPE" == "freebsd"* ]]; then
        echo "Implement autoinstall on FreeBSD"
        exit 1
    fi
done

# Check which directory we are in. Unless it's the base repo directory, fail.
if [ $CURRENT_DIR != "ei-freertos" ]; then
    echo "You should be in the base directory 'ei-freertos' when running this command."
    echo "Switch to it, and run this again."
    exit 1
fi

# Install required python libraries
pip3 install -r requirements.txt

# Initialise the submodules
git submodule update --init

# Check that a number of required folders are all on the path.
PATH_ERROR=0

if [[ ! :$PATH: == */"ei-freertos/tools"* ]] ; then
    echo -e "${RED}ERROR${NC}"
    echo "ei-freertos/tools isn't on the path"
    PATH_ERROR=1
fi

if [[ ! :$PYTHONPATH: == */"ei-freertos/pyclasses"* ]] ; then
    echo -e "${RED}ERROR${NC}"
    echo "ei-freertos/pyclasses isn't on the pythonpath"
    PATH_ERROR=1
fi

# Display a nice error message if the paths are not correctly set up.
if [ $PATH_ERROR == 1 ]; then
    echo -e "${YELLOW}Paths can setup correctly by adding the following lines to your .bash_profile:${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "CODE=/Users/{your_username_here}/code"
    else
        echo "CODE=/home/{your_username_here}/code"
    fi
    echo ""
    echo "export PATH=\$CODE/ei-freertos/tools:\$PATH"
    echo "export PYTHONPATH=\$PYTHONPATH:\$CODE/ei-freertos/pyclasses"
    echo ""
    exit 1
fi

if ! [ -x "$(command -v arm-none-eabi-gcc)" ]; then
    echo ""
    echo "${RED}ERROR${NC}"
    echo "You don't have arm-none-eabi-gcc currently installed."
    echo "To install it, use the following commands:"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "brew tap ArmMbed/homebrew-formulae"
        echo "brew install arm-none-eabi-gcc"
    else
        echo "${YELLOW}If you have already installed gcc-arm-none-eabi and you're having problems, uninstall it with:${NC}"
        echo "sudo apt-get remove gcc-arm-none-eabi"
        echo ""
        echo "${YELLOW}Then install the most up-to-date version with this command:${NC}"
        echo "sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa"
        echo "sudo apt-get update"
        echo "sudo apt-get install gcc-arm-embedded"
    fi
    echo ""
    exit 1
fi

# If the CPPUTEST folder doesn't exist, install cpputest
if [ ! -d $CURRENT_PATH/unit_tests/cpputest ]; then
    cd unit_tests
    git clone https://github.com/cpputest/cpputest.git
    cd cpputest
    ./autogen.sh
    mkdir a_build_dir && cd a_build_dir
    ../configure
    make
    make install
    make check
fi

# Setup is complete :)
cd $CURRENT_PATH
echo ""
echo -e "${GREEN}Repo Setup Completed Successfully${NC}"
