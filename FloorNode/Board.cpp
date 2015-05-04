/* 
 * File:   Board.cpp
 * Author: Cameron
 * 
 * Created on April 27, 2015, 10:43 PM
 */

#include "Board.h"

#include <avr/interrupt.h>

ISR(TIMER1_OVF_vect) {
 ComparatorCapSense<&PORTB,0>::timerOverflowHandler();
}

ISR(TIMER1_CAPT_vect) {
 ComparatorCapSense<&PORTB,0>::inputCaptureHandler();
}

// Even though this is a full static class, we define it here for convenience.
ComparatorCapSense<&PORTB,0> Board::capSense;
