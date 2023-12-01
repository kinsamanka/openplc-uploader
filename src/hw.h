#ifndef HW_H
#define HW_H

#include "config.h"

#if defined BOARD_STM32

/******************PINOUT CONFIGURATION***********************
Digital In:  PA8, PA11, PA12, PB3, PB4, PB5, (%IX0.0 - %IX0.7)
             PB8, PB9, PB10
Digital Out: PB11, PB12, PB13, PB14, PB15,   (%QX0.0 - %QX0.7)
             PC13, PC14, PC15
Analog In:   PA0, PA1, PA4, PA5, PA6, PA7    (%IW0 - %IW5)
Analog Out:  PB0, PB1                        (%QW0 - %QW1)
**************************************************************/

#ifndef DIN
#define DIN                         {PA8, PA11, PA12, PB3, PB4, PB5, PB8, PB9, PB10}
#endif
#ifndef AIN
#define AIN                         {PA0, PA1, PA4, PA5, PA6, PA7}
#endif
#ifndef DOUT
#define DOUT                        {PB11, PB12, PB13, PB14, PB15, PC13, PC14, PC15}
#endif
#ifndef AOUT
#define AOUT                        {PB0, PB1}
#endif

#define UART1                       Serial1
#define UART2                       Serial2

#ifndef RUN_SW
#define RUN_SW                      PB2
#endif

#define RS_485_EN                   PA14

#elif defined BOARD_FX3U_14

#undef DIN                          /* override uploader since pinout is fixed */
#undef DOUT
#undef AIN
#undef AOUT

#define DIN                         {PB13, PB14, PB11, PB12, PE15, PB10, PE13, PE14}
#define DOUT                        {PC9, PC8, PA8, PA0, PB3, PD12}
#define AIN                         {PA1, PA3, PC4, PC5, PC0, PC1, PC2, PC3}
#define AOUT                        {PA4, PA5}

#ifndef RUN_LED
#define RUN_LED                     PD10
#endif

#ifndef RUN_SW
#define RUN_SW                      PB2
#endif

#define UART1                       Serial1
#define UART2                       Serial3

#define RS_485_EN                   PA14

#elif defined BOARD_FX3U_24

#undef DIN                          /* override uploader since pinout is fixed */
#undef DOUT
#undef AIN
#undef AOUT

#define DIN                         {PB13, PB14, PB11, PB12, PE15, PB10, PE13, \
                                     PE14, PE11, PE12, PE9, PE10, PE7, PE8,    \
                                     PC7}
#define DOUT                        {PC9, PC8, PA8, PA0, PB3, PD12, PB15, PA7, \
                                     PA6, PA2}
#define AIN                         {PA1, PA3, PC4, PC5, PC0, PC1, PC2, PC3}
#define AOUT                        {PA4, PA5}

#ifndef RUN_SW
#define RUN_SW                      PB2
#endif

#ifndef RUN_LED
#define RUN_LED                     PD10
#endif

#define UART1                       Serial1
#define UART2                       Serial3

#define RS_485_EN                   PA14

#elif defined BOARD_FX3U_26_E

#undef DIN                          /* override uploader since pinout is fixed */
#undef DOUT
#undef AIN
#undef AOUT

#define DIN                         {PA5, PD9, PA10, PA11, PA12, PA13, PA14,  \
                                     PA15, PD0, PD1, PD2, PD3, PD4, PD5, PD6, \
                                     PD7}
#define DOUT                        {PE8, PE7, PB0, PE6, PE5, PE4, PE3, PE2,  \
                                     PE15, PE14}
#define AIN                         {PC3, PC4}
#define AOUT                        {PA4}

#ifndef RUN_LED
#define RUN_LED                     PE0
#endif
#ifndef ERR_LED
#define ERR_LED                     PE1
#endif

#ifndef RUN_SW
#define RUN_SW                      PB2
#endif

#define UART1                       Serial1
#define UART2                       Serial4

#define SPI_MISO                    PB14
#define SPI_MOSI                    PB15
#define SPI_SCLK                    PB13
#define SPI_SS                      PB12

#elif defined BOARD_ETH_MODBUS_IO5R

#undef DIN                          /* override uploader since pinout is fixed */
#undef DOUT
#undef AIN
#undef AOUT

#define DIN                         {PB1, PB12, PB13, PB14, PB15, PB8}
#define DOUT                        {PB5, PB6, PB7, PB10, PB11}
#define AIN                         {}
#define AOUT                        {}

#ifndef RUN_LED
#define RUN_LED                     PA8
#endif

#define UART1                       Serial1
#define UART2                       Serial2

#define RS_485_EN                   PA10

#define SPI_MISO                    PA6
#define SPI_MOSI                    PA7
#define SPI_SCLK                    PA5
#define SPI_SS                      PA4

#define W5500_NRST                  PA0

#else

#error "No BOARD defined!"

#endif

#if ARDUINO_ARCH_STM32 && defined STM32F1xx

#define UART_SR_TCn(n)               ( n ## _SR_TC )
#define UART_SR_TC(n)                UART_SR_TCn(n)

#define Serial1_SR_TC                (USART1->SR & USART_SR_TC)
#define Serial2_SR_TC                (USART2->SR & USART_SR_TC)
#define Serial3_SR_TC                (USART3->SR & USART_SR_TC)
#define Serial4_SR_TC                (USART4->SR & USART_SR_TC)

#endif

#if defined RS485_MASTER_EN && !defined RS485_MASTER_EN_PIN
#error "RS485 Master is enabled but no defined EN pin!"
#endif

#if defined RS485_SLAVE_EN && !defined RS485_SLAVE_EN_PIN
#error "RS485 Slave is enabled but no defined EN pin!"
#endif

#if defined RS485_SLAVE_EN && !defined UART_SLAVE_TX_COMPLETE
#error "RS485 Slave is enabled but no uart tx complete test defined!"
#endif

#if defined RS485_MASTER_EN && !defined UART_MASTER_TX_COMPLETE
#error "RS485 Master is enabled but no uart tx complete test defined!"
#endif


/* Defaults */

#ifndef RUN_LED
#define RUN_LED                     0
#endif

#ifndef ERR_LED
#define ERR_LED                     0
#endif

#ifndef RUN_SW
#define RUN_SW                      0
#define IS_RUN_SW                   1
#else
#define IS_RUN_SW                   digitalRead(RUN_SW)
#endif

#ifndef ARDUINO_ARCH_STM32
#define ARDUINO_ARCH_STM32          0
#endif

#ifndef SPI_MISO
#define SPI_MISO                    0
#endif

#ifndef SPI_MOSI
#define SPI_MOSI                    0
#endif

#ifndef SPI_SCLK
#define SPI_SCLK                    0
#endif

#ifndef SPI_SS
#define SPI_SS                      0
#endif

#ifndef W5500_NRST
#define W5500_NRST                  0
#endif

#ifndef RS_485_EN
#define RS_485_EN                   0
#endif

#endif
