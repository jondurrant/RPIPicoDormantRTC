/*
 * Dormant.h
 *
 *  Created on: 25 Jun 2024
 *      Author: jondurrant
 */

#ifndef SRC_DORMANT_H_
#define SRC_DORMANT_H_

#include "pico/stdlib.h"

class Dormant {
public:
	Dormant();

	void sleep(uint8_t wakePad);
	virtual ~Dormant();


private:
	void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig);


	 uint scb_orig;
	 uint clock0_orig;
	 uint clock1_orig;
};

#endif /* SRC_DORMANT_H_ */
