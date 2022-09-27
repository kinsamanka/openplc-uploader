#include <Arduino.h>

#include "config.h"
#include "hw.h"
#include "modbus.h"
#include "serial.h"

void serial_init(void)
{
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
void serial_slave_task(unsigned long dt)
{
    static enum states {
        INIT,
        RX,
        TX,
#ifdef RS485_SLAVE_EN
        TX_WAIT,
#endif
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
            if (process_master_request(&slave_buf)) {
#ifdef RS485_SLAVE_EN
                digitalWrite(RS485_EN_PIN, 1);
#endif
                state = TX;
            }
        }
        break;

    case TX:
        if (slave_buf.pos < slave_buf.len) {
            MBSLAVE_IFACE.write(slave_buf.data[slave_buf.pos++]);
        } else {
            slave_buf.len = 0;
#ifndef RS485_SLAVE_EN
            slave_buf.last_dt = dt;
            state = RX;
#else
            state = TX_WAIT;
#endif
        }
#ifdef RS485_SLAVE_EN
        break;
    case TX_WAIT:
        if (UART_TX_COMPLETE) {

            mb_master_buf.last_dt = dt;
            digitalWrite(RS485_EN_PIN, 0);

            state = RX;
        }
#endif
    }
}
#endif

#if MBMASTER
void serial_master_task(unsigned long dt)
{
    static enum states {
        INIT,
        TX_PREP,
        TX,
#ifdef RS485_MASTER_EN
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
#ifdef RS485_MASTER_EN
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
#ifndef RS485_MASTER_EN
            mb_master_buf.last_dt = dt;
            state = RX;
#else
            state = TX_WAIT;
#endif
        }
        break;

#ifdef RS485_MASTER_EN
    case TX_WAIT:
        if (UART_TX_COMPLETE) {

            mb_master_buf.last_dt = dt;
            digitalWrite(RS485_EN_PIN, 0);

            state = RX;
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
