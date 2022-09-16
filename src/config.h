#ifndef CONFIG_H
#define CONFIG_H

#define NUM(a) (sizeof(a) / sizeof(*a))
#define ct_assert(e) ((void)sizeof(char[1 - 2*!(e)]))

#ifndef MAX_REQUEST
#if RAM_SIZE > 8192
#define MAX_REQUEST             256
#else
#define MAX_REQUEST             64
#endif
#endif

#ifndef MAX_RESPONSE
#if RAM_SIZE > 8192
#define MAX_RESPONSE            256
#else
#define MAX_RESPONSE            64
#endif
#endif

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || \
    defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)

#define HOLDING_REG_COUNT       3
#define INPUT_REG_COUNT         6
#define COIL_COUNT              8
#define DISCRETE_COUNT          8

#ifdef MODBUS_MASTER
#undef MODBUS_MASTER
#endif

#endif

#ifndef HOLDING_REG_COUNT
#define HOLDING_REG_COUNT       16
#endif

#ifndef INPUT_REG_COUNT
#define INPUT_REG_COUNT         8
#endif

#ifndef COIL_COUNT
#define COIL_COUNT              24
#endif

#ifndef DISCRETE_COUNT
#define DISCRETE_COUNT          24
#endif

#ifdef MODBUS_MASTER

#undef MODBUS_MASTER
#define MODBUS_MASTER           1

#ifndef SLAVE_HOLDING_REG_COUNT
#define SLAVE_HOLDING_REG_COUNT 16
#endif

#ifndef SLAVE_INPUT_REG_COUNT
#define SLAVE_INPUT_REG_COUNT   16
#endif

#ifndef SLAVE_COIL_COUNT
#define SLAVE_COIL_COUNT        16
#endif

#ifndef SLAVE_DISCRETE_COUNT
#define SLAVE_DISCRETE_COUNT    16
#endif

#define QW_BASE                 HOLDING_REG_COUNT
#define IW_BASE                 INPUT_REG_COUNT
#define QX_BASE                 (COIL_COUNT / 8)
#define IX_BASE                 (DISCRETE_COUNT / 8)

#else

#define MODBUS_MASTER           0

#ifdef SLAVE_HOLDING_REG_COUNT
#undef SLAVE_HOLDING_REG_COUNT
#endif

#ifdef SLAVE_INPUT_REG_COUNT
#undef SLAVE_INPUT_REG_COUNT
#endif

#ifdef SLAVE_COIL_COUNT
#undef SLAVE_COIL_COUNT
#endif

#ifdef SLAVE_DISCRETE_COUNT
#undef SLAVE_DISCRETE_COUNT
#endif

#define SLAVE_HOLDING_REG_COUNT 0
#define SLAVE_INPUT_REG_COUNT   0
#define SLAVE_COIL_COUNT        0
#define SLAVE_DISCRETE_COUNT    0

#define QW_BASE                 0
#define IW_BASE                 0
#define QX_BASE                 0
#define IX_BASE                 0

#endif

#define QW_COUNT                (HOLDING_REG_COUNT + SLAVE_HOLDING_REG_COUNT)
#define IW_COUNT                (INPUT_REG_COUNT + SLAVE_INPUT_REG_COUNT)
#define QX_COUNT                (COIL_COUNT + SLAVE_COIL_COUNT)
#define IX_COUNT                (DISCRETE_COUNT + SLAVE_DISCRETE_COUNT)

#define MBSERIAL_IFACE Serial
#define MBSERIAL_BAUD 115200
#define MBTCP_MAC 0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD
#define MBTCP_IP 
#define MBTCP_DNS 
#define MBTCP_GATEWAY 
#define MBTCP_SUBNET 255,255,255,0
#define MBTCP_SSID ""
#define MBTCP_PWD ""

#endif
