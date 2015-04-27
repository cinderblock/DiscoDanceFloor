

#include "ComparatorCapSense.h"

template<volatile uint8_t * DrivePort, uint8_t drivePin>
bool ComparatorCapSense<DrivePort, drivePin>::initialized = false;

template<volatile uint8_t * DrivePort, uint8_t drivePin>
void ComparatorCapSense<DrivePort, drivePin>::init() {
 // Set PORT output low on drive line
 *DrivePort |= 1 << drivePin;

 // Set DDR as output for drive port
 *(DrivePort - 1) |= 1 << drivePin;

 initialized = true;
}

template<volatile uint8_t * DrivePort, uint8_t drivePin>
ComparatorCapSense<DrivePort, drivePin>::ComparatorCapSense() : mux(mux) {
 if (!initialized) init();


}
