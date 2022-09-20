#include "modbus_inc.h"

#define NUM(a) (sizeof(a) / sizeof(*a))

#define INIT_STRUCT(a,b,c,d)                                                  \
    {.address = a, .index = b, .count = c, .type = d, .next = NULL}

#define INIT_MULTIPLE(a,b,c,d,e)                                              \
    do {                                                                      \
        static struct mb_clients                                              \
            z = INIT_STRUCT(a, b, NUM(GetFbVar(c,.table)), e);                \
        mb_list_check(&z, (void *)GetFbVar(c,.table), &GetFbVar(d));          \
    } while(0)

#define INIT_SINGLE(a,b,c,d,e)                                                \
    do {                                                                      \
        static struct mb_clients z = INIT_STRUCT(a, b, 1, e);                 \
        mb_list_check(&z, (void *) &GetFbVar(c), &GetFbVar(d));               \
    } while(0)

#define MB_RTU_READ_COILS(addr, idx, buf, res)                                \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, READ_COILS);                   \
    } while(0)

#define MB_RTU_READ_DISCRETE_INPUTS(addr, idx, buf, res)                      \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, READ_DISCRETE_INPUTS);         \
    } while(0)

#define MB_RTU_READ_HOLDING_REGISTERS(addr, idx, buf, res)                    \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, READ_HOLDING_REGISTERS);       \
    } while(0)

#define MB_RTU_READ_INPUT_REGISTERS(addr, idx, buf, res)                      \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, READ_INPUT_REGISTERS);         \
    } while(0)

#define MB_RTU_WRITE_MULTIPLE_COILS(addr, idx, buf, res)                      \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, WRITE_COILS);                  \
    } while(0)

#define MB_RTU_WRITE_MULTIPLE_REGISTERS(addr, idx, buf, res)                  \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_MULTIPLE(addr, idx, buf, res, WRITE_HOLDING_REGISTERS);      \
    } while(0)

#define MB_RTU_WRITE_SINGLE_COIL(addr, idx, buf, res)                         \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_SINGLE(addr, idx, buf, res, WRITE_SINGLE_COIL);              \
    } while(0)

#define MB_RTU_WRITE_SINGLE_REGISTER(addr, idx, buf, res)                     \
    do {                                                                      \
        if (!mb_run_master)                                                   \
            INIT_SINGLE(addr, idx, buf, res, WRITE_SINGLE_REGISTER);          \
    } while(0)
