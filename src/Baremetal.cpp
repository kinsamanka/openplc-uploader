#include <Arduino.h>

#include "openplc.h"
#include "modbus.h"
#include "hw.h"

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;

void setup()
{
    hardwareInit();
    config_init__();

    scan_cycle = (uint32_t) (common_ticktime__ / 1000000);
    timer_ms = millis() + scan_cycle;

#if MBSLAVE || MBMASTER
    modbus_init();
#if MBSLAVE
    MBSLAVE_IFACE.begin(SLAVE_BAUD_RATE);
    MBSLAVE_IFACE.flush();
#endif
#if MBMASTER
    MBMASTER_IFACE.begin(MASTER_BAUD_RATE);
    MBMASTER_IFACE.flush();
#endif
#ifdef RS485_EN
    pinMode(RS485_EN_PIN, OUTPUT);
    digitalWrite(RS485_EN_PIN, 0);
#endif
#endif
}

#if MBSLAVE
static void serial_slave_task(unsigned long dt)
{
    static enum states {
        INIT,
        RX,
        TX,
    } state = INIT;

    static uint8_t data[UART_BUF_LEN];

    static struct modbus_buf slave_buf = {
        .data = data,
        .len = 0,
        .pos = 0,
        .size = UART_BUF_LEN,
        .last_dt = 0,
    };

    switch (state) {
    case INIT:
        slave_buf.len = 0;
        slave_buf.last_dt = dt;
        state = RX;
        /* fall through */

    case RX:
        if (MBSLAVE_IFACE.available()) {
            if (slave_buf.len < slave_buf.size)
                slave_buf.data[slave_buf.len++] = MBSLAVE_IFACE.read();
            else
                (void)MBSLAVE_IFACE.read();
            slave_buf.last_dt = dt;
        } else if ((dt - slave_buf.last_dt) > MB_SLAVE_TIMEOUT) {
            if (process_master_request(&slave_buf))
                state = TX;
        }
        break;

    case TX:
        if (slave_buf.pos < slave_buf.len) {
            MBSLAVE_IFACE.write(slave_buf.data[slave_buf.pos++]);
        } else {
            slave_buf.len = 0;
            slave_buf.last_dt = dt;
            state = RX;
        }
    }
}
#endif

#if MBMASTER
static void serial_master_task(unsigned long dt)
{
    static enum states {
        INIT,
        TX_PREP,
        TX,
#if defined RS485_EN && defined RS485_MASTER_EN
        TX_WAIT,
#endif
        RX,
    } state = INIT;

    static struct mb_clients *mbc = NULL;

    switch (state) {
    case INIT:
        if (start_master_task()) {

            mb_master_buf.len = 0;
            mb_master_buf.pos = 0;
            mb_master_buf.last_dt = dt;

            mbc = (struct mb_clients *)mb_head_master;

            state = TX_PREP;

        } else {

            break;
        }

    case TX_PREP:
        mbc = (struct mb_clients *)mbc->next;

        if (create_master_request(mbc, &mb_master_buf)) {
#if defined RS485_EN && defined RS485_MASTER_EN
            digitalWrite(RS485_EN_PIN, 1);
#endif
            state = TX;
        }

        break;

    case TX:
        if (mb_master_buf.pos < mb_master_buf.len) {

            MBMASTER_IFACE.write(mb_master_buf.data[mb_master_buf.pos++]);

        } else {

            mb_master_buf.len = 0;
            mb_master_buf.last_dt = dt;
#if defined RS485_EN && defined RS485_MASTER_EN
            state = TX_WAIT;
#else
            state = RX;
#endif
        }
        break;

#if defined RS485_EN && defined RS485_MASTER_EN
    case TX_WAIT:
        if ((MBMASTER_IFACE.availableForWrite() == 63) &&
            (dt - mb_master_buf.last_dt) > RS485_EN_TIMEOUT) {

            mb_master_buf.last_dt = dt;
            digitalWrite(RS485_EN_PIN, 0);

            state = RX;

        } else {

            mb_master_buf.last_dt = dt;

        }
        break;
#endif

    case RX:
        if (MBMASTER_IFACE.available()) {

            if (mb_master_buf.len < mb_master_buf.size)
                mb_master_buf.data[mb_master_buf.len++] = MBMASTER_IFACE.read();
            else
                (void)MBMASTER_IFACE.read();

            mb_master_buf.last_dt = dt;

        } else if ((dt - mb_master_buf.last_dt) > MB_MASTER_TIMEOUT) {

            process_slave_reply(mbc, &mb_master_buf);

            state = TX_PREP;
        }
    }
}
#endif

#define NUM_TASKS       (MBMASTER + MBSLAVE)

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
    };

    if (dt >= timer_ms) {

        /* PLC task has priority */
        timer_ms += scan_cycle;
        updateInputBuffers();
        config_run__(__tick++);
        updateOutputBuffers();
        updateTime();

    } else {
        if (cycle--)
            tasks[cycle](dt);
        else
            cycle = NUM_TASKS;
    }
}
