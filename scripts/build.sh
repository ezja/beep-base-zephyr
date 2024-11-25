#!/bin/bash

# Build script for BEEP Base firmware with nRF9161 support

# Exit on error
set -e

# Default values
BUILD_TYPE="release"
CLEAN=0
FLASH=0
FLASH_MODEM=0
DEBUG=0
VERBOSE=0
BOARD="nrf52840dk_nrf52840"
BUILD_DIR="build"
MODEM_FIRMWARE=""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Function to print usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -b, --board         Specify board (default: nrf52840dk_nrf52840)"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -d, --debug         Build in debug mode"
    echo "  -f, --flash         Flash the application after building"
    echo "  -m, --modem FILE    Flash modem firmware (specify firmware file)"
    echo "  -v, --verbose       Enable verbose output"
    echo "  -h, --help          Show this help message"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--board)
            BOARD="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -d|--debug)
            DEBUG=1
            BUILD_TYPE="debug"
            shift
            ;;
        -f|--flash)
            FLASH=1
            shift
            ;;
        -m|--modem)
            FLASH_MODEM=1
            MODEM_FIRMWARE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            ;;
    esac
done

# Check environment
check_environment() {
    echo -e "${YELLOW}Checking environment...${NC}"
    
    if [ -z "$ZEPHYR_BASE" ]; then
        echo -e "${RED}Error: ZEPHYR_BASE not set${NC}"
        exit 1
    fi
    
    if [ -z "$GNUARMEMB_TOOLCHAIN_PATH" ]; then
        echo -e "${RED}Error: GNUARMEMB_TOOLCHAIN_PATH not set${NC}"
        exit 1
    }
    
    # Verify Python script
    if [ ! -f "scripts/verify_nrf.py" ]; then
        echo -e "${RED}Error: verify_nrf.py not found${NC}"
        exit 1
    }
    
    # Run verification script
    python3 scripts/verify_nrf.py ${VERBOSE} && {
        echo -e "${GREEN}Environment check passed${NC}"
    } || {
        echo -e "${RED}Environment check failed${NC}"
        exit 1
    }
}

# Clean build directory
clean_build() {
    if [ $CLEAN -eq 1 ]; then
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf $BUILD_DIR
    fi
}

# Configure build
configure_build() {
    echo -e "${YELLOW}Configuring build...${NC}"
    
    # Create build directory
    mkdir -p $BUILD_DIR
    
    # Configure CMake
    cmake -B $BUILD_DIR -GNinja \
        -DBOARD=$BOARD \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE^^} \
        ${DEBUG:+-DCONFIG_DEBUG_BUILD=y} \
        ${VERBOSE:+-DCMAKE_VERBOSE_MAKEFILE=ON}
}

# Build application
build_app() {
    echo -e "${YELLOW}Building application...${NC}"
    cmake --build $BUILD_DIR ${VERBOSE:+--verbose}
}

# Flash application
flash_app() {
    if [ $FLASH -eq 1 ]; then
        echo -e "${YELLOW}Flashing application...${NC}"
        west flash -d $BUILD_DIR
    fi
}

# Flash modem firmware
flash_modem() {
    if [ $FLASH_MODEM -eq 1 ]; then
        if [ ! -f "$MODEM_FIRMWARE" ]; then
            echo -e "${RED}Error: Modem firmware file not found: $MODEM_FIRMWARE${NC}"
            exit 1
        fi
        
        echo -e "${YELLOW}Flashing modem firmware...${NC}"
        nrfjprog --program $MODEM_FIRMWARE --sectorerase
        nrfjprog --reset
    fi
}

# Main build process
main() {
    echo -e "${YELLOW}Starting build process...${NC}"
    
    # Run all steps
    check_environment
    clean_build
    configure_build
    build_app
    flash_modem
    flash_app
    
    echo -e "${GREEN}Build process completed successfully${NC}"
}

# Run main process
main
