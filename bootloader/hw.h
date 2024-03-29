#ifndef HW_H
#define HW_H

#include <stdint.h>

struct gpios {
    const uint32_t port;
    const uint16_t pin;
};

struct leds {
    const struct gpios gpio;
    const int inv;
};

struct uarts {
    const uint8_t index;
    const uint32_t port;
    const uint16_t rx;
    const uint16_t tx;
    const uint32_t remaps;
    const struct gpios en;
};

void adc_setup(void);
void adc_start(void);
void clock_setup(void);
void dac_setup(void);
void gpio_setup(void);
void run_bootloader(void);
void spi_setup(void);
void timer3_setup(void);
void update_inputs(void);
void update_ouputs(void);

#ifndef UPLOAD_SPEED
#define UPLOAD_SPEED                57600
#endif

#endif
