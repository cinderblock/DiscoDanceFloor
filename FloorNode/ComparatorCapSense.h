/**
 * Capacitive sensing based on AVR's internal analog comparator, timer, and the
 * input capture functionality
 *
 */

#ifndef ComparatorCapSense_H
#define ComparatorCapSense_H

#include <stdint.h>

template<volatile uint8_t * DrivePort, uint8_t drivePin>
class ComparatorCapSense {
 static bool initialized;

 static void init();

 static void startSequence();

 void setupComparatorMux();

 const uint8_t mux;

public:
 ComparatorCapSense(uint8_t const mux);

 void startScan();

 static bool isResultReady();

 static uint16_t getResult();
};

#endif // ComparatorCapSense_H
