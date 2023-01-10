#include <Arduino.h>

#include "async.h"
#include "config.h"
#include "hw.h"
#include "modbus.h"
#include "serial.h"

static uint8_t mb_slave_data[UART_BUF_LEN];
static uint8_t mb_master_data[UART_BUF_LEN];

struct serial_slave_state {
    async_state;
    struct modbus_buf mb;
};

struct serial_master_state {
    async_state;
    struct modbus_buf mb;
    struct mb_clients *clients;
};

static struct serial_slave_state slave_serial_state = {
    0,
    .mb = {
           .data = mb_slave_data,
           .len = 0,
           .pos = 0,
           .size = UART_BUF_LEN,
           .last_dt = 0,
           .rtu = 1,
           }
};

static struct serial_master_state master_serial_state = {
    0,
    .mb = {
           .data = mb_master_data,
           .len = 0,
           .pos = 0,
           .size = UART_BUF_LEN,
           .last_dt = 0,
           .rtu = 1,
           },
    .clients = NULL,
};

void serial_init(void)
{
    if (MBSLAVE) {
        MBSLAVE_IFACE.begin(SLAVE_BAUD_RATE);
        MBSLAVE_IFACE.flush();
    }

    if (MBMASTER) {
        MBMASTER_IFACE.begin(MASTER_BAUD_RATE);
        MBMASTER_IFACE.flush();
    }

    if (RS485_MASTER_EN) {
        pinMode(RS485_MASTER_EN_PIN, OUTPUT);
        digitalWrite(RS485_MASTER_EN_PIN, 0);
    }

    if (RS485_SLAVE_EN) {
        pinMode(RS485_SLAVE_EN_PIN, OUTPUT);
        digitalWrite(RS485_SLAVE_EN_PIN, 0);
    }

    if (MBSLAVE)
        async_init(&slave_serial_state);
    if (MBMASTER)
        async_init(&master_serial_state);
}

static inline void slave_en_pin(int state)
{
    if (RS485_SLAVE_EN)
        digitalWrite(RS485_SLAVE_EN_PIN, state);
}

static async serial_slave(unsigned long dt, struct serial_slave_state *pt)
{
    uint8_t c;
    async_begin(pt);

    pt->mb.len = 0;
    pt->mb.pos = 0;
    pt->mb.last_dt = dt;

    while (1) {
        /* wait for request */
        while (1) {
            if (MBSLAVE_IFACE.available()) {

                if (pt->mb.len < pt->mb.size)
                    pt->mb.data[pt->mb.len++] = MBSLAVE_IFACE.read();
                else
                    (void)MBSLAVE_IFACE.read();

                pt->mb.last_dt = dt;
            }

            if ((dt - pt->mb.last_dt) > MB_SLAVE_TIMEOUT)
                if (process_master_request(&pt->mb))
                    break;

            async_yield;
        }

        slave_en_pin(1);

        /* send reply */
        while (pt->mb.pos < pt->mb.len) {
            c = pt->mb.data[pt->mb.pos++];
            MBSLAVE_IFACE.write(c);
            async_yield;
        }

        await(!RS485_SLAVE_EN || UART_SLAVE_TX_COMPLETE);

        slave_en_pin(0);

        pt->mb.len = 0;
        pt->mb.last_dt = dt;
    }

    async_end;
}

static inline void master_en_pin(int state)
{
    if (RS485_MASTER_EN)
        digitalWrite(RS485_MASTER_EN_PIN, state);
}

static async serial_master(unsigned long dt, struct serial_master_state *pt)
{
    async_begin(pt);

    pt->mb.len = 0;
    pt->mb.pos = 0;
    pt->mb.last_dt = dt;

    while (!start_master_task()) {
        async_yield;
    }

    pt->clients = (struct mb_clients *)mb_head_master;

    while (1) {
        pt->clients = (struct mb_clients *)pt->clients->next;

        if (!create_master_request(pt->clients, &pt->mb))
            continue;

        master_en_pin(1);

        /* send request */
        while (pt->mb.pos < pt->mb.len) {
            MBMASTER_IFACE.write(pt->mb.data[pt->mb.pos++]);
            async_yield;
        }

        await(!RS485_MASTER_EN || UART_MASTER_TX_COMPLETE);

        master_en_pin(0);

        pt->mb.len = 0;
        pt->mb.last_dt = dt;

        /* receive reply */
        do {
            if (MBMASTER_IFACE.available()) {
                if (pt->mb.len < pt->mb.size)
                    pt->mb.data[pt->mb.len++] = MBMASTER_IFACE.read();
                else
                    (void)MBMASTER_IFACE.read();

                pt->mb.last_dt = dt;
            }

            async_yield;

        } while ((dt - pt->mb.last_dt) < MB_MASTER_TIMEOUT);

        process_slave_reply(pt->clients, &pt->mb);

        async_yield;
    }

    async_end;
}

void serial_task(unsigned long dt)
{
    if (MBSLAVE)
        serial_slave(dt, &slave_serial_state);
    if (MBMASTER)
        serial_master(dt, &master_serial_state);
}
