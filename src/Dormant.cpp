/*
 * Dormant.cpp
 *
 *  Created on: 25 Jun 2024
 *      Author: jondurrant
 */

#include "Dormant.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/rtc.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

Dormant::Dormant() {
	 uint scb_orig = scb_hw->scr;
	 uint clock0_orig = clocks_hw->sleep_en0;
	 uint clock1_orig = clocks_hw->sleep_en1;
}

Dormant::~Dormant() {
	// TODO Auto-generated destructor stub
}


void Dormant::sleep(uint8_t wakePad){

	sleep_run_from_xosc();
	sleep_goto_dormant_until_pin(wakePad, true, false);
	recover_from_sleep(scb_orig, clock0_orig, clock1_orig);
}
