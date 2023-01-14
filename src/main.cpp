#include <Arduino.h>

#include "async.h"
#include "openplc.h"
#include "modbus.h"
#include "hw.h"
#include "serial.h"
#include "wifi.h"
#include "ethernet.h"

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;

int run_prog;

struct task_state {
    async_state;
} ts;

struct blink_task_state {
    async_state;
    unsigned long dt;
} bts;

void setup()
{
    hardwareInit();
    config_init__();

    scan_cycle = (uint32_t) (common_ticktime__ / 1000000);
    timer_ms = millis() + scan_cycle;

    if (MBSLAVE || MBMASTER || MBWIFI || MBETH)
        modbus_init();

    if (MBSLAVE || MBMASTER || ARDUINO_ARCH_STM32)
        serial_init();

    if (MBWIFI)
        wifi_init();

    if (MBETH)
        eth_init();

    async_init(&ts);
    async_init(&bts);
}

static async blink_task(unsigned long dt)
{
    async_begin(&bts);
    
    bts.dt = dt;
    
    while (1) {
        digitalWrite(RUN_LED, HIGH);
        await((dt - bts.dt) > 300);

        await(run_prog);

        digitalWrite(RUN_LED, LOW);
        await((dt - bts.dt) > 900);

        bts.dt = dt;
    }

    async_end;
}

static async run_tasks(unsigned long dt)
{
    async_begin(&ts);

    while (1) {

        if (RUN_LED)
            await_while(blink_task(dt));

        if (MBSLAVE || MBMASTER || ARDUINO_ARCH_STM32) {
            serial_task(dt, run_prog);
            async_yield;
        }

        if (MBWIFI) {
            wifi_task();
            async_yield;
        }

        if (MBETH) {
            eth_task();
            async_yield;
        }
    }

    async_end;
}

void loop()
{
    unsigned long dt = millis();

    run_prog = IS_RUN_SW;

    if (dt >= timer_ms) {

        timer_ms += scan_cycle;

        if (run_prog) {
            updateInputBuffers();
            config_run__(__tick++);
            updateOutputBuffers();
            updateTime();
        } else {
            __tick = 0;
        }
    }

    run_tasks(dt);
}
