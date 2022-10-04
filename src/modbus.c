#include <string.h>
#include "modbus.h"

#if MBMASTER
#define LIGHTMODBUS_FULL
#else
#define LIGHTMODBUS_SLAVE_FULL
#endif
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

extern uint16_t QW[];
extern uint16_t IW[];
extern uint8_t QX[];
extern uint8_t IX[];

static ModbusSlave slave;
static int slave_err = 0;

#if MBMASTER
static ModbusMaster master;

uint8_t mb_run_master = 0;

void *mb_head_master = NULL;

struct modbus_buf mb_master_buf = {
        .data = NULL,
        .len = 0,
        .pos = 0,
        .size = 0,
        .last_dt = 0,
};

static struct mb_buffer {
    uint8_t *data;
    size_t size;
} mb_bits, mb_buf;
#endif

static ModbusError slave_allocator(ModbusBuffer * buffer, uint16_t size,
                                   void *context)
{
    (void)context;
    static uint8_t buf[MAX_RESPONSE];

    if (size != 0) {
        if (size <= MAX_RESPONSE) {
            buffer->data = buf;
            return MODBUS_OK;
        } else {
            buffer->data = NULL;
            return MODBUS_ERROR_ALLOC;
        }
    } else {
        buffer->data = NULL;
        return MODBUS_OK;
    }
}

static ModbusError slave_CB(const ModbusSlave * status,
                            const ModbusRegisterCallbackArgs * args,
                            ModbusRegisterCallbackResult * result)
{
    (void)status;

    int size = 0;

    switch (args->query) {

    case MODBUS_REGQ_R_CHECK:
        switch (args->type) {

        case MODBUS_HOLDING_REGISTER:
            size = QW_COUNT;
            break;

        case MODBUS_INPUT_REGISTER:
            size = IW_COUNT;
            break;

        case MODBUS_COIL:
            size = QX_COUNT;
            break;

        case MODBUS_DISCRETE_INPUT:
            size = IX_COUNT;
            break;
        }

        if (args->index < size)
            result->exceptionCode = MODBUS_EXCEP_NONE;
        else
            result->exceptionCode = MODBUS_EXCEP_ILLEGAL_ADDRESS;
        break;

    case MODBUS_REGQ_W_CHECK:
        switch (args->type) {

        case MODBUS_HOLDING_REGISTER:
            size = QW_COUNT;
            break;

        case MODBUS_COIL:
            size = QX_COUNT;
            break;

        default:
            size = 0;
        }

        if (args->index < size)
            result->exceptionCode = MODBUS_EXCEP_NONE;
        else
            result->exceptionCode = MODBUS_EXCEP_SLAVE_FAILURE;
        break;

    case MODBUS_REGQ_R:
        switch (args->type) {

        case MODBUS_HOLDING_REGISTER:
            result->value = QW[args->index];
            break;

        case MODBUS_INPUT_REGISTER:
            result->value = IW[args->index];
            break;

        case MODBUS_COIL:
            result->value = QX[args->index];
            break;

        case MODBUS_DISCRETE_INPUT:
            result->value = IX[args->index];
            break;
        }
        break;

    case MODBUS_REGQ_W:
        switch (args->type) {

        case MODBUS_HOLDING_REGISTER:
            QW[args->index] = args->value;
            break;

        case MODBUS_COIL:
            QX[args->index] = args->value;
            break;

        default:
            break;
        }
        break;
    }

    return MODBUS_OK;
}

static ModbusError slave_exception_CB(const ModbusSlave * status,
                                      uint8_t function,
                                      ModbusExceptionCode code)
{
    (void)status;
    (void)function;
    (void)code;

    slave_err = 1;
    return MODBUS_OK;
}

int process_master_request(struct modbus_buf *buf)
{
    int result = 0;
    /* reset global error flag */
    slave_err = 0;

    if (buf->len) {
        ModbusErrorInfo err;

        if (buf->rtu) {
            err = modbusParseRequestRTU(&slave, SLAVE_ADDRESS,
                                        (const uint8_t *)buf->data, buf->len);
        } else {
            err = modbusParseRequestTCP(&slave,
                                        (const uint8_t *)buf->data, buf->len);
        }

        if (modbusIsOk(err)) {

            uint16_t sz;
            /* no reply on broadcast messages */
            if ((sz = modbusSlaveGetResponseLength(&slave))) {
                uint8_t *dat = (uint8_t *) modbusSlaveGetResponse(&slave);

                buf->len = sz;
                memcpy(buf->data, dat, sz);

                result = 1;
            }
        }
    }

    if (slave_err)
        result = 0;

    if (!result)
        buf->len = 0;

    buf->pos = 0;

    return result;
}

#if MBMASTER
static ModbusError master_allocator(ModbusBuffer * buffer, uint16_t size,
                                    void *context)
{
    (void)context;

    if (size != 0) {
        if (size <= mb_buf.size) {
            buffer->data = mb_buf.data;
            return MODBUS_OK;
        } else {
            buffer->data = NULL;
            return MODBUS_ERROR_ALLOC;
        }
    } else {
        buffer->data = NULL;
        return MODBUS_OK;
    }
}

static uint8_t CB_index = 0;
static void *CB_data = NULL;

static ModbusError master_CB(const ModbusMaster * status,
                             const ModbusDataCallbackArgs * args)
{
    if (args->function == 1 || args->function == 2) {

        ((uint8_t *) CB_data)[CB_index++] = args->value;

    } else if (args->function == 3 || args->function == 4) {

        ((uint16_t *) CB_data)[CB_index++] = args->value;
    }

    return MODBUS_OK;
}

static ModbusError master_exception_CB(const ModbusMaster * status,
                                       uint8_t address, uint8_t function,
                                       ModbusExceptionCode code)
{
    (void)status;
    (void)address;
    (void)function;
    (void)code;

    return MODBUS_OK;
}

int create_master_request(struct mb_clients *mbc, struct modbus_buf *buf)
{
    ModbusErrorInfo err;

    switch (mbc->type) {
    case READ_COILS:
        err = modbusBuildRequest01RTU(&master, mbc->address, mbc->index,
                                      mbc->count);
        break;
    case READ_DISCRETE_INPUTS:
        err = modbusBuildRequest02RTU(&master, mbc->address, mbc->index,
                                      mbc->count);
        break;
    case READ_HOLDING_REGISTERS:
        err = modbusBuildRequest03RTU(&master, mbc->address, mbc->index,
                                      mbc->count);
        break;
    case READ_INPUT_REGISTERS:
        err = modbusBuildRequest04RTU(&master, mbc->address, mbc->index,
                                      mbc->count);
        break;
    case WRITE_COILS:
        memset(mb_bits.data, 0, mb_bits.size);

        /* convert bit array to bits */
        for (uint8_t i = 0; i < mbc->count; i++) {
            if (((uint8_t *)mbc->data)[i])
                mb_bits.data[i / 8] |= 1 << (i % 8);
        }

        err = modbusBuildRequest15RTU(&master, mbc->address, mbc->index,
                                      mbc->count,
                                      (const uint8_t *)mb_bits.data);
        break;
    case WRITE_HOLDING_REGISTERS:
        err = modbusBuildRequest16RTU(&master, mbc->address, mbc->index,
                                      mbc->count, (const uint16_t *)mbc->data);
        break;
    case WRITE_SINGLE_COIL:
        err = modbusBuildRequest05RTU(&master, mbc->address, mbc->index,
                                      *((uint8_t *) mbc->data) ? 1 : 0);
        break;
     case WRITE_SINGLE_REGISTER:
        err = modbusBuildRequest06RTU(&master, mbc->address, mbc->index,
                                      *(uint16_t *) mbc->data);
    }

    if (!modbusIsOk(err))
        return buf->len = buf->pos = 0;

    int sz = modbusMasterGetRequestLength(&master);
    memcpy(buf->data, (uint8_t *) modbusMasterGetRequest(&master), sz);
    buf->len = sz;
    buf->pos = 0;

    return sz;
}

void process_slave_reply(struct mb_clients *mbc, struct modbus_buf *buf)
{
    *mbc->ok = 0;
    CB_index = 0;
    CB_data = mbc->data;

    if (buf->len) {
        ModbusErrorInfo err;
        err = modbusParseResponseRTU(&master,
                                     modbusMasterGetRequest(&master),
                                     modbusMasterGetRequestLength
                                     (&master), buf->data, buf->len);
        if (modbusIsOk(err))
            *mbc->ok = 1;
    }
}

int start_master_task(void)
{
    if (mb_run_master && mb_head_master) {

        struct mb_clients *el, *last;
        size_t n = 0, m = 0;

        /* calculate size of modbus buffers */
        for (el = mb_head_master; el; el = el->next) {
            switch(el->type) {
            case READ_COILS:
            case READ_DISCRETE_INPUTS:
            case WRITE_COILS:
                if ((el->count / 8) > n)
                    n = el->count / 8;
                if (el->count > m)
                    m = el->count;
                break;
            case READ_HOLDING_REGISTERS:
            case READ_INPUT_REGISTERS:
            case WRITE_HOLDING_REGISTERS:
                if (el->count > n)
                    n = el->count;
                break;
            default:
                break;
            }

            last = el;
        }
        /* convert to circular list */
        last->next = mb_head_master;

        n++;                /* n, max registers */
        n = (n * 2) + 9;    /* max mb message   */
        m--;                /* m, max # of bits */
        m = (m / 8) + 1;    /* max # of bytes   */

        mb_master_buf.data = malloc(n);

        mb_buf.data = malloc(n);
        mb_buf.size = n;

        mb_bits.data = malloc(m);
        mb_bits.size = m;

        if (mb_master_buf.data && mb_buf.data && mb_bits.data)
            return 1;
    }

    return 0;
}

void mb_list_check(void *elem, void *data, uint8_t *ok)
{
    struct mb_clients *y;

    /* check if element is already in the list */
    for (y = mb_head_master; y; y = y->next) {
        /* if it exists then this is already the 2nd cycle */
        mb_run_master |= (y == elem);
    }

    if (!mb_run_master) {
        y = elem;
        y->ok = ok;
        y->data = data;

        /* prepend to list */
        y->next = mb_head_master;
        mb_head_master = y;
    }
}
#endif

void modbus_init(void)
{
    ModbusErrorInfo err;

    err = modbusSlaveInit(&slave,
                          slave_CB,
                          slave_exception_CB,
                          slave_allocator,
                          modbusSlaveDefaultFunctions,
                          modbusSlaveDefaultFunctionCount);

#if MBMASTER
    err = modbusMasterInit(&master,
                           master_CB,
                           master_exception_CB,
                           master_allocator,
                           modbusMasterDefaultFunctions,
                           modbusMasterDefaultFunctionCount);
#endif
    (void)err;
}
