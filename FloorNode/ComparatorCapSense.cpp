

#include "ComparatorCapSense.h"

template<volatile uint8_t * DrivePort, uint8_t drivePin>
uint8_t ComparatorCapSense<DrivePort, drivePin>::mux;

template<volatile uint8_t * DrivePort, uint8_t drivePin>
bool ComparatorCapSense<DrivePort, drivePin>::initialized;



template<volatile uint8_t * DrivePort, uint8_t portPinNumber>
void ComparatorCapSense<DrivePort, portPinNumber>::init() {
 // Set PORT output low on drive line
 *DrivePort |= 1 << portPinNumber;

 // Set DDR as output for drive port
 *(DrivePort - 1) |= 1 << portPinNumber;
 
 initialized = true;
 
 // Clear timer state (but set the correct edge selection on the input capture)
 TCCR1A = 0;
 TCCR1B = 0 << ICES1;
 
 // Reset timer's counter
 TCNT1 = 0;
 
 // Clear all interrupt flags
 TIFR1 = 0xff;
 
 // Enable overflow and input capture interrupts
 TIMSK1 = 1 << ICIE1 | 1 << TOIE1;
 
 // Disable the Analog to Digital converter
 ADCSRA = 0;
 // Enable using the ADC multiplexer for inverting (-) input
 ADCSRB = 1 << ACME;
 
 // 1.1V Bandgap reference on non-inverting (+) input and connect output to timer
 ACSR = 1 << ACBG | 1 << ACIC;
}

template<volatile uint8_t * DrivePort, uint8_t portPinNumber>
ComparatorCapSense<DrivePort, portPinNumber>::ComparatorCapSense(const uint8_t newMux) {
 mux = newMux;
 if (initialized) return;
 
 init();
}


template<volatile uint8_t * DrivePort, uint8_t portPinNumber>
void ComparatorCapSense<DrivePort, portPinNumber>::timerOverflowHandler() {
 
}

template<volatile uint8_t * DrivePort, uint8_t portPinNumber>
void ComparatorCapSense<DrivePort, portPinNumber>::startScan() {
 
 
 
}

