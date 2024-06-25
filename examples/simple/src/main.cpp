/**
 * Simple Hibernate and Recovery
 */

#include "pico/stdlib.h"
#include "DS3231.hpp"
#include "Dormant.h"
#include "hardware/i2c.h"
#include <cstdio>


#define LED_PAD 2
#define DELAY 500 // in microseconds
#define SDA_PAD 12
#define SCL_PAD 13

#define WAKE_PAD 10


void flash(uint count=1){
	const uint LED_PIN = LED_PAD;

	for (uint i=0; i < count; i++){
		gpio_put(LED_PIN, 1);
		sleep_ms(DELAY);
		gpio_put(LED_PIN, 0);
		sleep_ms(DELAY);
	}
}


int main() {
	uint resurrect = 0;
    stdio_init_all();
    sleep_ms(2000);
    printf("GO\n");

    const uint LED_PIN = LED_PAD;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(WAKE_PAD);
    gpio_pull_up(WAKE_PAD);
    gpio_set_dir(WAKE_PAD, GPIO_IN);

    DS3231 rtc(i2c0,  SDA_PAD,  SCL_PAD);
    printf("RTC: %s\n", rtc.get_time_str());

    flash(20);

    Dormant dormant;

    rtc.set_delay(1);
    dormant.sleep(WAKE_PAD);

    while (true) { // Loop forever



    	uint8_t pad = gpio_get(WAKE_PAD);
    	printf("Pad %u at Time: %s\n",
    			pad,
    			rtc.get_time_str());
    	if (pad == 0){
    		resurrect++;
    		printf("RESSURECT %u\n", resurrect);

    		flash(20);
    		 rtc.set_delay(1);
    		dormant.sleep(WAKE_PAD);
    	}

    	sleep_ms(DELAY);

    }

}
