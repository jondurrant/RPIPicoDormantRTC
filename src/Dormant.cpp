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
	  scb_orig = scb_hw->scr;
	  clock0_orig = clocks_hw->sleep_en0;
	  clock1_orig = clocks_hw->sleep_en1;
}

Dormant::~Dormant() {
	// TODO Auto-generated destructor stub
}


void Dormant::sleep(uint8_t wakePad){

	sleep_run_from_xosc();
	sleep_goto_dormant_until_pin(wakePad, true, false);
	recover_from_sleep(scb_orig, clock0_orig, clock1_orig);
}

void Dormant::recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_LSB);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    stdio_init_all();

    return;
}
