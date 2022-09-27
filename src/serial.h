#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
void serial_slave_task(unsigned long dt);
void serial_master_task(unsigned long dt);

#endif
