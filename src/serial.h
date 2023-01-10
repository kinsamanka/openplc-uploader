#ifndef SERIAL_H
#define SERIAL_H

#ifdef RS485_MASTER_EN
#undef RS485_MASTER_EN
#define RS485_MASTER_EN             1
#else
#define RS485_MASTER_EN             0
#endif

#ifdef RS485_SLAVE_EN
#undef RS485_SLAVE_EN
#define RS485_SLAVE_EN              1
#else
#define RS485_SLAVE_EN              0
#endif

#ifndef RS485_SLAVE_EN_PIN
#define RS485_SLAVE_EN_PIN          0
#endif

#ifndef RS485_MASTER_EN_PIN
#define RS485_MASTER_EN_PIN         0
#endif

#ifndef UART_SLAVE_TX_COMPLETE
#define UART_SLAVE_TX_COMPLETE      1
#endif

#ifndef UART_MASTER_TX_COMPLETE
#define UART_MASTER_TX_COMPLETE     1
#endif

void serial_init(void);
void serial_task(unsigned long dt);

#endif
