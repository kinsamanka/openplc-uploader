enum modbus_cmds {
    READ_COILS,
    READ_DISCRETE_INPUTS,
    READ_HOLDING_REGISTERS,
    READ_INPUT_REGISTERS,
    WRITE_COILS,
    WRITE_HOLDING_REGISTERS,
    WRITE_SINGLE_COIL,
    WRITE_SINGLE_REGISTER,
};

struct mb_clients {
    uint8_t address, count, type, *ok;
    uint16_t index;
    void *data, *next;
};

extern uint8_t mb_run_master;

extern void *mb_head_master;

void mb_list_check(void *elem, void *data, uint8_t *ok);
