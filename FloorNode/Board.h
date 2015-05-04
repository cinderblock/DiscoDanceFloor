/* 
 * File:   Board.h
 * Author: Cameron
 *
 * Created on April 27, 2015, 10:43 PM
 * 
 * This file ideally specifies all of the constants needed for this application
 * that could change if the PCB/wiring changes.
 */

#ifndef BOARD_H
#define	BOARD_H

#include "ComparatorCapSense.h"

#include <avr/io.h>

constexpr volatile uint8_t * const PortC = &PORTC;

volatile uint8_t &PC = *PortC;


namespace Board {

 /*
  * The new capacitive sensing library's handle.
  */
 extern ComparatorCapSense<&PORTC, 0> capSense;
};

#endif	/* BOARD_H */

