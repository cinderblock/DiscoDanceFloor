
# List of CPP file basenames to build
CPP = FloorNode MessageBuffer TestMaster ComparatorCapSense Board

# Add required libraries to the list of files ot build
CPP += libraries/CapacitiveSensor/CapacitiveSensor
CPP += libraries/LEDFader/LEDFader libraries/LEDFader/Curve

# Add our "libraries" to the include path
INCLUDES += libraries/CapacitiveSensor libraries/LEDFader libraries/EEPROM

# Frequency the AVR is running at
F_CPU = 8000000UL

MCU = atmega328p

all: build #run

run: avrdude-flash

TARGET = DanceNode

uMakerPath = uMaker/

# Load local settings
-include local.mk

# Force setting certain make flags
include $(uMakerPath)tools/makeflags.mk

# Optional configuration testing for development
include $(uMakerPath)tools/checks.mk

# Defs for our setup
include $(uMakerPath)vars/AVR.mk

# Library targets
include $(uMakerPath)tools/AVR/Arduino.mk
#include $(uMakerPath)tools/AVR/lufa.mk
#include $(uMakerPath)tools/nRF51/lib.mk
#include $(uMakerPath)tools/extlib.mk
#include $(uMakerPath)tools/FreeRTOS.mk

# Build targets
include $(uMakerPath)tools/build.mk

# Programmer targets
include $(uMakerPath)tools/AVR/avrdude.mk

# Directory creation targets
include $(uMakerPath)tools/mkdir.mk

.PHONY: all run
