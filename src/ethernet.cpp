#include <Arduino.h>

#include "async.h"
#include "config.h"
#include "hw.h"

#if MBETH

#include <SPI.h>
#include <Ethernet.h>

#include "ethernet.h"
#include "modbus.h"

static EthernetServer server(CONFIG_MODBUS_PORT);
static uint8_t data[MAX_RESPONSE];

struct eth_slave_state {
    async_state;
    struct modbus_buf mb;
    EthernetClient clients[CONFIG_MAX_TCP_CONN];
    size_t client_idx;
};

static struct eth_slave_state eth_slave_state = {
    0,
    .mb = {
           .data = data,
           .len = 0,
           .pos = 0,
           .size = MAX_RESPONSE,
           .last_dt = 0,
           .rtu = 0,
           },
};

void eth_init(void)
{
#if SPI_MISO && SPI_MOSI && SPI_SCLK && SPI_SS
    SPI.setMISO(SPI_MISO);
    SPI.setMOSI(SPI_MOSI);
    SPI.setSCLK(SPI_SCLK);

    SPI.begin(SPI_SS);
#endif

    if (SPI_SS)
        Ethernet.init(SPI_SS);

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

    async_init(&eth_slave_state);
}

static async eth_slave(struct eth_slave_state *pt)
{
    size_t i, n;
    EthernetClient client;

    client = server.accept();
    if (client) {
        for (i = 0; i < CONFIG_MAX_TCP_CONN; i++)
            if (!pt->clients[i].connected()) {
                pt->clients[i] = client;
                break;
            }
        if (i == CONFIG_MAX_TCP_CONN)
            client.stop();
    }

    async_begin(pt);

    pt->client_idx = 0;

    while (1) {

        client = pt->clients[pt->client_idx];
        if (client) {
            if (client.connected()) {
                n = client.available();
                if (n) {
                    for (i = 0; i < n; i++) {
                        if (i < pt->mb.size)
                            pt->mb.data[i] = client.read();
                        else
                            client.read();
                    }
                    if (n > pt->mb.size)
                        pt->mb.len = pt->mb.size;
                    else
                        pt->mb.len = n;

                    if (process_master_request(&pt->mb))
                        client.write(pt->mb.data, pt->mb.len);
                }
            } else {
                client.stop();
            }
        }

        if (++pt->client_idx >= CONFIG_MAX_TCP_CONN)
            pt->client_idx = 0;

        async_yield;
    }

    async_end;
}

void eth_task(void)
{
    if (MBETH)
        eth_slave(&eth_slave_state);
}

#endif                          /* MBETH */
