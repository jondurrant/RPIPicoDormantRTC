/**
 * Deep Sleep and Recovery on a Raspberry PI Pico
 * LED is flashed on GPIO 2 while a wake
 *
 * Use Internal RTC for recovery
 *
 * Count while sleep and awake using PWM on GPIO 15
 */

#include "pico/stdlib.h"
#include "DeepSleep.h"
#include <cstdio>
#include "hardware/gpio.h"
#include "hardware/pwm.h"


#define LED_PAD 2
#define DELAY 500 // in microseconds
#define COUNT_PAD 15

void flash(uint count=1){
	const uint LED_PIN = LED_PAD;

	for (uint i=0; i < count; i++){
		gpio_put(LED_PIN, 1);
		sleep_ms(DELAY);
		gpio_put(LED_PIN, 0);
		sleep_ms(DELAY);
	}
}

uint slice_num;
void setupPwmCount(uint8_t gp){
    gpio_init(gp);
    gpio_pull_up(gp);
    gpio_set_dir(gp, GPIO_IN);

    if (pwm_gpio_to_channel(gp) != PWM_CHAN_B){
    	printf("ERROR - GPIO Must be PWM Channel B\n");
    }
   slice_num = pwm_gpio_to_slice_num(gp);

   // Count once for every 1 cycles the PWM B input is HIGH
   pwm_config cfg = pwm_get_default_config();
   pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_FALLING);
   pwm_config_set_clkdiv(&cfg, 1);
   pwm_init(slice_num, &cfg, false);
   gpio_set_function(gp, GPIO_FUNC_PWM);
   pwm_set_enabled(slice_num, true);
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


    setupPwmCount(COUNT_PAD);

    flash(10);

    //Drop into initial sleep for 1 minute
    DeepSleep* deepSleep = DeepSleep::singleton();
    deepSleep->enablePWM();

    while (true) { // Loop forever

        printf("SLEEP: Count at %d\n",  pwm_get_counter(slice_num));
        uart_default_tx_wait_blocking();

        deepSleep->sleepMin(1);

		resurrect++;
		printf("RESSURECT %u Count at %d\n",
				resurrect,
				pwm_get_counter(slice_num)
				);

		flash(5);

    }

}
