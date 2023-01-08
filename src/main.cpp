#include <Arduino.h>

#include "openplc.h"
#include "modbus.h"
#include "serial.h"
#include "wifi.h"
#include "ethernet.h"

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;

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
}

#define NUM_TASKS       (MBMASTER + MBSLAVE + MBWIFI + MBETH)

void loop()
{
    unsigned long dt = millis();

    static int8_t cycle = NUM_TASKS;
    typedef void (*f) (unsigned long);

    /*
     * this is an implementation of a simple
     * multitasker, running a single task at
     * every loop cycle
     *
     */

    const f tasks[] = {
#if MBSLAVE
        serial_slave_task,
#endif
#if MBMASTER
        serial_master_task,
#endif
#if MBWIFI
        wifi_slave_task,
#endif
#if MBETH
        eth_slave_task,
#endif
   };

    if (dt >= timer_ms) {

        /* PLC task has priority */
        timer_ms += scan_cycle;
        updateInputBuffers();
        config_run__(__tick++);
        updateOutputBuffers();
        updateTime();

    } else if (NUM_TASKS) {
        if (cycle--)
            tasks[cycle](dt);
        else
            cycle = NUM_TASKS;
    }
}
