/**
 * Capacitive sensing based on AVR's internal analog comparator, timer, and the
 * input capture functionality.
 * 
 * 
 * Eventually, I'd like this class to handle multiple cap sense inputs from one 
 *
 */

#ifndef ComparatorCapSense_H
#define ComparatorCapSense_H

#include <avr/interrupt.h>

#include <stdint.h>

// Interrupt stubs
ISR(TIMER1_OVF_vect);
ISR(TIMER1_CAPT_vect);

template<volatile uint8_t * DrivePort, uint8_t portPinNumber>
class ComparatorCapSense {
 static bool initialized;

 static void init();

 static void startSequence();

 static void setupComparatorMux();

 static uint8_t mux;

public:
 /**
  * Setup an instance of this object to watch for when a particular channel on
  * the analog multiplexer crosses the 1.1V internal bandgap reference voltage.
  * 
  * @param mux The value to pass to the analog multiplexer for this channel.
  * This usually matches the ADC number on the AVR's pinout description.
  */
 ComparatorCapSense(uint8_t const mux);
 
 static void timerOverflowHandler();
 
 static void inputCaptureHandler();

 static void startScan();

 static bool isResultReady();

 static uint16_t getResult();
};

#endif // ComparatorCapSense_H
