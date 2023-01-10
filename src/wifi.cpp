#include <Arduino.h>

#include "async.h"
#include "config.h"
#include "hw.h"

#if MBWIFI

#if defined USE_ESP8266
#include <ESP8266WiFi.h>
#elif defined USE_WIFININA
#include <WiFiNINA.h>
#else
#include <WiFi.h>
#endif

#include "wifi.h"
#include "modbus.h"

WiFiServer server(CONFIG_MODBUS_PORT);
static uint8_t data[MAX_RESPONSE];

struct wifi_slave_state {
    async_state;
    struct modbus_buf mb;
    WiFiClient clients[CONFIG_MAX_TCP_CONN];
};

static struct wifi_slave_state wifi_slave_state = {
    0,
    .mb = {
           .data = data,
           .len = 0,
           .pos = 0,
           .size = MAX_RESPONSE,
           .last_dt = 0,
           .rtu = 0,
           }
};

void wifi_init(void)
{
    const char* ssid = CONFIG_SSID;
    const char* pass = CONFIG_WIFI_PASS;

#if defined CONFIG_WIFI_IP
    const uint8_t ip[] = CONFIG_WIFI_IP;
#if defined CONFIG_WIFI_GW
    const uint8_t gw[] = CONFIG_WIFI_GW;
#elif !defined USE_WIFININA
    const uint8_t gw[] = CONFIG_WIFI_IP;
#endif
#if defined CONFIG_WIFI_SUBNET
    const uint8_t sn[] = CONFIG_WIFI_SUBNET;
#endif
#endif

#if defined CONFIG_WIFI_DNS
    const uint8_t dns[] = CONFIG_WIFI_DNS;
#endif

#if defined USE_WIFININA && defined CONFIG_WIFI_IP && defined CONFIG_WIFI_DNS \
    && defined CONFIG_WIFI_GW && defined CONFIG_WIFI_SUBNET
    WiFi.config(ip, dns, gw, sn);
#elif defined USE_WIFININA && defined CONFIG_WIFI_IP && defined CONFIG_WIFI_DNS \
      && defined CONFIG_WIFI_GW
    WiFi.config(ip, dns, gw);
#elif defined USE_WIFININA && defined CONFIG_WIFI_IP && defined CONFIG_WIFI_DNS
    WiFi.config(ip, dns);
#elif defined USE_WIFININA && defined CONFIG_WIFI_IP
    WiFi.config(ip);
#elif defined CONFIG_WIFI_IP && defined CONFIG_WIFI_DNS
    WiFi.config(ip, gw, sn, dns);
#elif defined CONFIG_WIFI_IP
    WiFi.config(ip, gw, sn);
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    server.begin();

    async_init(&wifi_slave_state);
}

static async wifi_slave(struct wifi_slave_state *pt)
{
    WiFiClient client;
    uint8_t i;

    async_begin(pt);

    while (1) {
        client = server.available();

        if (client) {
            for (uint8_t i = 0; i < CONFIG_MAX_TCP_CONN; i++)
                if (!pt->clients[i]) {
                    pt->clients[i] = client;
                    break;
                }
        }

        async_yield;

        for (i = 0; i < CONFIG_MAX_TCP_CONN; i++) {

            client = pt->clients[i];
            if (client) {
                if (!client.connected()) {
                    client.stop();
                } else {

                    size_t n = client.available();
                    if (n) {
                        pt->mb.len = client.read(pt->mb.data, n);
                        if (process_master_request(&pt->mb))
                            client.write(pt->mb.data, pt->mb.len);
                    }
                }
            }

            async_yield;
        }
    }

    async_end;
}

void wifi_task(void)
{
    if (MBWIFI)
        wifi_slave(&wifi_slave_state);
}

#endif          /* MBWIFI */
