/*
 * Dormant.h
 *
 * Put Pico into Dormant mode and recover. Wake through GPIO and normally a DS3231 RTC alarm
 *
 * based on experiments in https://github.com/ghubcoder/PicoSleepRtc
 *
 *  Created on: 25 Jun 2024
 *      Author: jondurrant
 */

#ifndef SRC_DORMANT_H_
#define SRC_DORMANT_H_

#include "pico/stdlib.h"
#include "DS3231.hpp"

class Dormant {
public:
	Dormant();

	/***
	 * Constructor with RTC
	 * If no RTC will just ignore RTC comms
	 * @param rtc - pointer to the RTC object. Can be NULL
	 */
	Dormant(DS3231 *rtc);

	/***
	 * Sleep until pad pulled to ground
	 * @param wakePad - GPIO Pad for wake
	 */
	void sleep(uint8_t wakePad);

	/***
	 * Sleep for number of minutes and wake by GPIO pad
	 * If no RTC then it will just do sleep(wakePad)
	 * @param minutes - Minutes to sleep for
	 * @param wakePad - GPIO Pad for wake
	 */
	void sleep(uint minutes, uint8_t wakePad);


	virtual ~Dormant();


private:
	/***
	 * Reset the clocks
	 * @param scb_orig
	 * @param clock0_orig
	 * @param clock1_orig
	 */
	void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig);

	/***
	 * Store the clocks
	 */
	void storeClocks();

	DS3231 *pRTC = NULL;
	 uint scb_orig;
	 uint clock0_orig;
	 uint clock1_orig;
};

#endif /* SRC_DORMANT_H_ */
