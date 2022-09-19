#include "openplc.h"
#include "modbus.h"

unsigned long __tick = 0;

unsigned long scan_cycle;
unsigned long timer_ms = 0;

#include "arduino_libs.h"

void setup() 
{   
    hardwareInit();
    config_init__();

    scan_cycle = (uint32_t)(common_ticktime__/1000000);
    timer_ms = millis() + scan_cycle;

#ifdef MODBUS_ENABLED
    modbus_init();
#ifdef MBSERIAL
    MBSERIAL_IFACE.begin(SLAVE_BAUD_RATE);
    while (!MBSERIAL_IFACE);
    MBSERIAL_IFACE.flush();
#endif
#endif
}

void loop() 
{
    static enum states {
        INIT,
        RX,
        TX,
        RUN,
    } state = INIT;

    static uint8_t data[UART_BUF_LEN];

    static struct modbus_buf rx_buf = {
        .data = data,
        .len = 0,
        .pos = 0,
        .last_rx = 0,
    };

    enum states oldstate;
    unsigned long t = millis();

    if (t >= timer_ms) {
        timer_ms += scan_cycle;
        oldstate = state;
        state = RUN;
    }

    switch (state) {
    case INIT:
        rx_buf.len = 0;
        rx_buf.last_rx = t;
        state = RX;
        /* fall through */

    case RX:
        if (MBSERIAL_IFACE.available()) {
            if (rx_buf.len < UART_BUF_LEN)
                rx_buf.data[rx_buf.len++] = MBSERIAL_IFACE.read();
            else
                (void) MBSERIAL_IFACE.read();
            rx_buf.last_rx = t;
        } else if ((t - rx_buf.last_rx) > 2) {
            if (process_request(&rx_buf))
                state = TX;
        }
        break;

    case TX:
        if (rx_buf.pos < rx_buf.len) {
            MBSERIAL_IFACE.write(rx_buf.data[rx_buf.pos++]);
        } else {
            rx_buf.len = 0;
            rx_buf.last_rx = t;
            state = RX;
        }
        break;

    case RUN:
        updateInputBuffers();
        config_run__(__tick++);
        updateOutputBuffers();
        updateTime();
        state = oldstate;
    }
}
