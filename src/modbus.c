#define LIGHTMODBUS_SLAVE_FULL
#define LIGHTMODBUS_IMPL
#include <lightmodbus/lightmodbus.h>

#include <string.h>
#include "config.h"
#include "modbus.h"

extern uint16_t QW[];
extern uint16_t IW[];
extern uint8_t QX[];
extern uint8_t IX[];

static ModbusSlave slave;
static int slave_err = 0;

static ModbusError static_allocator(ModbusBuffer * buffer, uint16_t size,
                                    uint8_t * buf)
{
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

static ModbusError slave_allocator(ModbusBuffer * buffer, uint16_t size,
                                   void *context)
{
    (void)context;
    static uint8_t buf[MAX_RESPONSE];

    return static_allocator(buffer, size, buf);
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

int process_request(struct modbus_buf *buf)
{
    int result = 0;
    /* reset global error flag */
    slave_err = 0;

    if (buf->len) {
        ModbusErrorInfo err;
        err = modbusParseRequestRTU(&slave, SLAVE_ADDRESS,
                                    (const uint8_t *)buf->data, buf->len);

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

void modbus_init(void)
{
    ModbusErrorInfo err;

    err = modbusSlaveInit(&slave,
                          slave_CB,
                          slave_exception_CB,
                          slave_allocator,
                          modbusSlaveDefaultFunctions,
                          modbusSlaveDefaultFunctionCount);

    (void)err;
}
