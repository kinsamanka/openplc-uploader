#include <Arduino.h>

#include "async.h"
#include "openplc.h"
#include "modbus.h"
#include "serial.h"
#include "wifi.h"
#include "ethernet.h"

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;

struct task_state {
    async_state;
} pt;

void setup()
{
    hardwareInit();
    config_init__();

    scan_cycle = (uint32_t) (common_ticktime__ / 1000000);
    timer_ms = millis() + scan_cycle;

    if (MBSLAVE || MBMASTER || MBWIFI || MBETH)
        modbus_init();

    if (MBSLAVE || MBMASTER)
        serial_init();

    if (MBWIFI)
        wifi_init();

    if (MBETH)
        eth_init();

    async_init(&pt);
}

static async run_tasks(unsigned long dt)
{
    async_begin(&pt);

    while (1) {

        if (MBSLAVE || MBMASTER) {
            serial_task(dt);
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

    if (dt >= timer_ms) {

        timer_ms += scan_cycle;
        updateInputBuffers();
        config_run__(__tick++);
        updateOutputBuffers();
        updateTime();

    }

    run_tasks(dt);
}
