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
	  storeClocks();
}

Dormant::Dormant(DS3231 *rtc) {
	pRTC = rtc;
	storeClocks();
}

void Dormant::setRTC(DS3231 *rtc){
	pRTC = rtc;
}

void Dormant::storeClocks(){
	  scb_orig = scb_hw->scr;
	  clock0_orig = clocks_hw->sleep_en0;
	  clock1_orig = clocks_hw->sleep_en1;
}

Dormant::~Dormant() {
	// TODO Auto-generated destructor stub
}


void Dormant::sleep(uint8_t wakePad){
	gpio_init(wakePad);
	gpio_pull_up(wakePad);
	gpio_set_dir(wakePad, GPIO_IN);

	sleep_run_from_xosc();
	sleep_goto_dormant_until_pin(wakePad, true, false);
	recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

	gpio_disable_pulls(wakePad);
}

void Dormant::sleep(uint minutes, uint8_t wakePad){
	notifyObservers(minutes, false);
	if (pRTC != NULL){
		pRTC->clear_alarm();
		pRTC->set_delay(minutes);
	}
	sleep(wakePad);
	if (pRTC != NULL){
		pRTC->clear_alarm();
	}
	notifyObservers(minutes, true);
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


Dormant * Dormant::pSingleton  = NULL;
Dormant * Dormant::singleton(){
	if (pSingleton == NULL){
		pSingleton = new Dormant;
	}
	return pSingleton;
}

void Dormant::addObserver(DormantNotification *obs){
	xObservers.push_back(obs);
}

void Dormant::delObserver(DormantNotification *obs){
	xObservers.remove(obs);
}

void Dormant::notifyObservers(uint minutes, bool wake){
	for (auto itr = xObservers.begin();
	        itr != xObservers.end(); itr++) {
			if (!wake) {
				(*itr)->notifyDormant(minutes);
			} else {
				(*itr)->notifyWake(minutes);
			}
	    }
}


