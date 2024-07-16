/*
 * DeepSleep.cpp
 *
 *  Created on: 16 Jul 2024
 *      Author: jondurrant
 */

#include "DeepSleep.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/rtc.h"
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

DeepSleep::DeepSleep() {
	 storeClocks();

}

DeepSleep::~DeepSleep() {
	// TODO Auto-generated destructor stub
}

void DeepSleep::setRTC(DS3231 *rtc){
	pRTC = rtc;
}

void gpio_callback(uint gpio, uint32_t events) {

}

void DeepSleep::sleep(uint8_t wakePad){
	gpio_init(wakePad);
	gpio_pull_up(wakePad);
	gpio_set_dir(wakePad, GPIO_IN);
	//gpio_set_irq_enabled_with_callback (
	gpio_set_irq_enabled(
			wakePad,
		   GPIO_IRQ_EDGE_FALL |
		   GPIO_IRQ_EDGE_RISE,
		   true
		   //&gpio_callback
		   );

	sleep_run_from_xosc();
	sleep_until_interupt();
	recover_from_sleep(scb_orig, clock0_orig, clock1_orig);

	gpio_disable_pulls(wakePad);
}

void DeepSleep::rtcCB(void) {
	; //NOP
}


void DeepSleep::sleep(uint minutes, uint8_t wakePad){
	notifyObservers(minutes, false);
	if (pRTC != NULL){
		pRTC->clear_alarm();
		pRTC->set_delay(minutes);
	} else {
		if (!rtc_running()){
			rtc_init();
			 datetime_t t = {
			            .year  = 2020,
			            .month = 06,
			            .day   = 05,
			            .dotw  = 5, // 0 is Sunday, so 5 is Friday
			            .hour  = 15,
			            .min   = 45,
			            .sec   = 00
			    };
			 rtc_set_datetime(&t);
		}
		datetime_t t;
		if (!rtc_get_datetime (&t)){
			printf("RTC Broken\n");
			uart_default_tx_wait_blocking();
		}
		t.year = -1;
		t.month = -1;
		t.day = -1;
		t.hour = -1;
		t.dotw = -1;
		t.min = (t.min + minutes) %60;
		t.min = -1;
		t.sec = (t.sec + 10) % 60;
		rtc_set_alarm ( &t,  DeepSleep::rtcCB);
	}
	sleep(wakePad);
	if (pRTC != NULL){
		pRTC->clear_alarm();
	}
	notifyObservers(minutes, true);
}

void DeepSleep::recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

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

void DeepSleep::sleep_until_interupt( ) {
    // Turn off all clocks when in sleep mode except for RTC
	if (pRTC == NULL){
		clocks_hw->sleep_en0 =
				CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS
				| CLOCKS_SLEEP_EN0_CLK_SYS_PWM_BITS;
	} else {
		clocks_hw->sleep_en0 =
				CLOCKS_SLEEP_EN0_CLK_SYS_PWM_BITS;
	}
    clocks_hw->sleep_en1 = 0x0;

    uint save = scb_hw->scr;
    // Enable deep sleep at the proc
    scb_hw->scr = save | M0PLUS_SCR_SLEEPDEEP_BITS;

    // Go to sleep
    __wfi();
}




DeepSleep * DeepSleep::pSingleton  = NULL;
DeepSleep * DeepSleep::singleton(){
	if (pSingleton == NULL){
		pSingleton = new DeepSleep;
	}
	return pSingleton;
}

void DeepSleep::addObserver(DormantNotification *obs){
	xObservers.push_back(obs);
}

void DeepSleep::delObserver(DormantNotification *obs){
	xObservers.remove(obs);
}

void DeepSleep::notifyObservers(uint minutes, bool wake){
	for (auto itr = xObservers.begin();
	        itr != xObservers.end(); itr++) {
			if (!wake) {
				(*itr)->notifyDormant(minutes);
			} else {
				(*itr)->notifyWake(minutes);
			}
	    }
}

void DeepSleep::storeClocks(){
	  scb_orig = scb_hw->scr;
	  clock0_orig = clocks_hw->sleep_en0;
	  clock1_orig = clocks_hw->sleep_en1;
}