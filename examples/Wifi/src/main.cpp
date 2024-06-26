/**
 * Simple Hibernate and Recovery on a Raspberry PI Pico
 * LED is flashed on GPIO 2 while a wake
 * Connects to WIFI and prints IP Address
 * Will deinit the Wifi when sleeping
 *
 * RTC DS3231 connected on I2C to GP12 & 13
 * RTC SQW used for interupt to wake on GP10
 */

#include "pico/stdlib.h"
#include "DS3231.hpp"
#include "Dormant.h"
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "WifiHelper.h"


#define LED_PAD 2
#define DELAY 500 // in microseconds
#define SDA_PAD 12
#define SCL_PAD 13

#define WAKE_PAD 10


void flash(uint count=1){
	const uint LED_PIN = LED_PAD;

	for (uint i=0; i < count; i++){
		gpio_put(LED_PIN, 1);
		cyw43_arch_poll();
		sleep_ms(DELAY);
		gpio_put(LED_PIN, 0);
		cyw43_arch_poll();
		sleep_ms(DELAY);
	}
}

void wifiConnect(){
	 //Connect to WIFI
	if (WifiHelper::init()){
		printf("Wifi Controller Initialised\n");
	} else {
		printf("Failed to initialise controller\n");
		return ;
	}
	printf("Connecting to WiFi... %s \n", WIFI_SSID);
	if (WifiHelper::join(WIFI_SSID, WIFI_PASSWORD)){
		printf("Connect to Wifi\n");
	} else {
		printf("Failed to connect to Wifi \n");
	}

	//Print IP Address
	char ipStr[20];
	WifiHelper::getIPAddressStr(ipStr);
	printf("IP ADDRESS: %s\n", ipStr);
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


    wifiConnect();
    flash(20);

    //Drop into initial sleep for 1 minute
    WifiHelper::deInit();
    Dormant *dormant = Dormant::singleton();
    dormant->setRTC(&rtc);
    printf("SLEEP\n");
    uart_default_tx_wait_blocking();
    dormant->sleep(1, WAKE_PAD);


    while (true) { // Loop forever
    	 wifiConnect();

    	//Print GPIO of Wake Pin
    	uint8_t pad = gpio_get(WAKE_PAD);


		resurrect++;
		printf("RESSURECT %u\n", resurrect);

		flash(5);

		//Sleep again
		printf("SLEEP\n");
		uart_default_tx_wait_blocking();
		WifiHelper::deInit();
		dormant->sleep(1, WAKE_PAD);

    	sleep_ms(DELAY);

    }

    return 0;

}
