/**
 * Deep Sleep and Recovery on a Raspberry PI Pico
 * LED is flashed on GPIO 2 while a wake
 *
 * Use external RTC for recovery
 *
 * RTC DS3231 connected on I2C to GP12 & 13
 * RTC SQW used for interupt to wake on GP10
 */

#include "pico/stdlib.h"
#include "DS3231.hpp"
#include "DeepSleep.h"
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

    //Setup LED
    const uint LED_PIN = LED_PAD;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //Set up RTC and get time
    DS3231 rtc(i2c0,  SDA_PAD,  SCL_PAD);
    printf("RTC: %s\n", rtc.get_time_str());


    flash(10);

    //Drop into initial sleep for 1 minute
    DeepSleep* deepSleep = DeepSleep::singleton();
    deepSleep->setRTC(&rtc);

    while (true) { // Loop forever

    	    printf("SLEEP\n");
    	    uart_default_tx_wait_blocking();
    	    deepSleep->sleep(1, WAKE_PAD);

    		resurrect++;
    		printf("RESSURECT %u\n", resurrect);

    		flash(5);

    }

}
