#ifndef MODBUS_H
#define MODBUS_H

struct modbus_buf {
    uint8_t *data;
    size_t len;
    size_t pos;
    unsigned long last_rx;
};

#ifdef __cplusplus
extern "C" {
#endif

void modbus_init(void);
int process_request(struct modbus_buf *buf);

#ifdef __cplusplus
}
#endif
#endif
