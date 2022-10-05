#ifndef CONFIG_H
#define CONFIG_H

#define NUM(a) (sizeof(a) / sizeof(*a))
#define ct_assert(e) ((void)sizeof(char[1 - 2*!(e)]))

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega32U4__)

#define HOLDING_REG_COUNT       8
#define INPUT_REG_COUNT         8
#define COIL_COUNT              8
#define DISCRETE_COUNT          8

#endif

#ifndef HOLDING_REG_COUNT
#define HOLDING_REG_COUNT       16
#endif

#ifndef INPUT_REG_COUNT
#define INPUT_REG_COUNT         16
#endif

#ifndef COIL_COUNT
#define COIL_COUNT              24
#endif

#ifndef DISCRETE_COUNT
#define DISCRETE_COUNT          24
#endif

#define QW_COUNT                HOLDING_REG_COUNT
#define IW_COUNT                INPUT_REG_COUNT
#define QX_COUNT                COIL_COUNT
#define IX_COUNT                DISCRETE_COUNT

#ifndef SLAVE_ADDRESS
#define SLAVE_ADDRESS           1
#endif

#ifndef SLAVE_BAUD_RATE
#define SLAVE_BAUD_RATE         57600
#endif

#ifndef MBSLAVE_IFACE
#define MBSLAVE_IFACE           Serial
#endif

#define MB_SLAVE_TIMEOUT        5

#ifdef MODBUS_SLAVE
#define MBSLAVE                 1
#else
#define MBSLAVE                 0
#endif

#ifdef MODBUS_MASTER

#define MBMASTER                1

#ifndef MBMASTER_IFACE
#define MBMASTER_IFACE          Serial1
#endif

#ifndef MASTER_BAUD_RATE
#define MASTER_BAUD_RATE        57600
#endif

#define MB_MASTER_TIMEOUT       50

#else

#define MBMASTER                0

#endif      /* MODBUS_MASTER */

#if defined RS485_MASTER_EN || defined RS485_SLAVE_EN
#define RS485_EN
#endif

/* calculate UART buffer length */
#if IW_COUNT > QW_COUNT
#define W_MAX                   IW_COUNT
#else
#define W_MAX                   QW_COUNT
#endif

#if IX_COUNT > QX_COUNT
#define X_MAX                   IX_COUNT
#else
#define X_MAX                   QX_COUNT
#endif

#ifndef UART_BUF_LEN
#if (X_MAX / 8) > W_MAX
#define UART_BUF_LEN            ((X_MAX / 4) + 9)
#else
#define UART_BUF_LEN            ((W_MAX * 2) + 9)
#endif
#endif

#ifndef MAX_RESPONSE
#define MAX_RESPONSE            UART_BUF_LEN
#endif

#ifdef MODBUS_WIFI
#define MBWIFI                  1
#else
#define MBWIFI                  0
#endif

#ifdef MODBUS_ETH
#define MBETH                   1
#else
#define MBETH                   0
#endif

/*

   Setup the following defines to configure IP settings:

    #define CONFIG_IP               { 10, 1, 1, 2 }
    #define CONFIG_GW               { 10, 1, 1, 1 }
    #define CONFIG_DNS              { 10, 1, 1, 1 }

*/

#ifndef CONFIG_MAC
#define CONFIG_MAC              { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }
#endif

#ifndef CONFIG_SUBNET
#define CONFIG_SUBNET           { 255, 255, 255, 0 }
#endif

/*

   Setup the following defines to configure WIFI IP settings:

    #define CONFIG_WIFI_IP          { 10, 1, 1, 2 }
    #define CONFIG_WIFI_GW          { 10, 1, 1, 1 }
    #define CONFIG_WIFI_DNS         { 10, 1, 1, 1 }

*/

#ifndef CONFIG_WIFI_SUBNET
#define CONFIG_WIFI_SUBNET      { 255, 255, 255, 0 }
#endif

#ifndef CONFIG_SSID
#define CONFIG_SSID             "OpenPLC"
#endif

#ifndef CONFIG_WIFI_PASS
#define CONFIG_WIFI_PASS        "wifiPassword"
#endif

#ifndef CONFIG_MODBUS_PORT
#define CONFIG_MODBUS_PORT      502
#endif

#ifndef CONFIG_MAX_TCP_CONN
#define CONFIG_MAX_TCP_CONN     4
#endif

#endif
