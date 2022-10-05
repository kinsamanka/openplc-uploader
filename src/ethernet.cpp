#include <Arduino.h>

#include "config.h"
#include "hw.h"

#if MBETH

#include <SPI.h>
#include <Ethernet.h>

#include "ethernet.h"
#include "modbus.h"

EthernetServer server(CONFIG_MODBUS_PORT);

EthernetClient clients[CONFIG_MAX_TCP_CONN];

void eth_init(void)
{
    uint8_t mac[] = CONFIG_MAC;
#if defined CONFIG_IP
    const uint8_t ip[] = CONFIG_IP;
#endif
#if defined CONFIG_GW
    const uint8_t gw[] = CONFIG_GW;
#endif
#if defined CONFIG_SUBNET
    const uint8_t sn[] = CONFIG_SUBNET;
#endif
#if defined CONFIG_DNS
    const uint8_t dns[] = CONFIG_DNS;
#endif

#if defined CONFIG_IP && defined CONFIG_DNS && defined CONFIG_GW && defined CONFIG_SUBNET
    Ethernet.begin(mac, ip, dns, gw, sn);
#elif defined CONFIG_IP && defined CONFIG_DNS && defined CONFIG_GW
    Ethernet.begin(mac, ip, dns, gw);
#elif defined CONFIG_IP && defined CONFIG_DNS
    Ethernet.begin(mac, ip, dns);
#elif defined CONFIG_IP
    Ethernet.begin(mac, ip);
#else
    Ethernet.begin(mac);
#endif

    server.begin();
}

void eth_slave_task(unsigned long dt)
{
    (void) dt;

    static uint8_t cur = 0;
    static uint8_t data[MAX_RESPONSE];

    static struct modbus_buf slave_buf = {
        .data = data,
        .len = 0,
        .pos = 0,
        .size = MAX_RESPONSE,
        .last_dt = 0,
        .rtu = 0,
    };

    static enum states {
        RX,
        TX,
    } state = RX;

    EthernetClient client = server.accept();

    if (client) {
        for (uint8_t i = 0; i < CONFIG_MAX_TCP_CONN; i++)
            if (!clients[i]) {
                clients[i] = client;
                break;
            }
    } else {
        switch (state) {
        case RX:
            if (clients[cur]) {
                size_t n = clients[cur].available();
                if (n) {
                    slave_buf.len = clients[cur].read(slave_buf.data, n);
                    if (process_master_request(&slave_buf)) {
                        state = TX;
                        break;
                    }
                } else if (!clients[cur].connected()) {
                    clients[cur].stop();
                }
            }

            cur++;
            if (cur == CONFIG_MAX_TCP_CONN)
                cur = 0;

            break;

        case TX:
            clients[cur].write(slave_buf.data, slave_buf.len);

            cur++;
            if (cur == CONFIG_MAX_TCP_CONN)
                cur = 0;

            state = RX;
        }
    }
}

#endif          /* MBETH */
