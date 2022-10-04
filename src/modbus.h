#ifndef MODBUS_H
#define MODBUS_H

#include "config.h"
#include <stdint.h>

struct modbus_buf {
    uint8_t *data;
    size_t len;
    size_t pos;
    size_t size;
    unsigned long last_dt;
    const uint8_t rtu;
};

#if MBMASTER
#include "modbus_inc.h"

extern struct modbus_buf mb_master_buf;

#endif

#ifdef __cplusplus
extern "C" {
#endif

void modbus_init(void);
int process_master_request(struct modbus_buf *buf);

#if MBMASTER
int create_master_request(struct mb_clients *mbc, struct modbus_buf *buf);
void process_slave_reply(struct mb_clients *mbc, struct modbus_buf *buf);
int start_master_task(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
