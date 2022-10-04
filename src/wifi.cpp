#include <Arduino.h>

#include "config.h"
#include "hw.h"

#if MBWIFI

#ifdef BOARD_ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "wifi.h"
#include "modbus.h"

WiFiServer server(CONFIG_MODBUS_PORT);

WiFiClient clients[CONFIG_MAX_TCP_CONN];

void wifi_init(void)
{
    const char* ssid = CONFIG_SSID;
    const char* pass = CONFIG_WIFI_PASS;

#if defined CONFIG_WIFI_IP
    const uint8_t ip[] = CONFIG_WIFI_IP;
#if defined CONFIG_WIFI_GW
    const uint8_t gw[] = CONFIG_WIFI_GW;
#else
    const uint8_t gw[] = CONFIG_WIFI_IP;
#endif
#if defined CONFIG_WIFI_SUBNET
    const uint8_t sn[] = CONFIG_WIFI_SUBNET;
#endif
#endif

#if defined CONFIG_WIFI_DNS
    const uint8_t dns[] = CONFIG_WIFI_DNS;
#endif

#if defined CONFIG_WIFI_IP && defined CONFIG_WIFI_DNS
    WiFi.config(ip, gw, sn, dns);
#elif defined CONFIG_WIFI_IP
    WiFi.config(ip, gw, sn);
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    server.begin();
}

void wifi_slave_task(unsigned long dt)
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

    WiFiClient client = server.available();

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

#endif          /* MBWIFI */
