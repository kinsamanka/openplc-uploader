FUNCTION_BLOCK MB_write_multiple_registers
  VAR_INPUT
    IN : ARRAY [0..7] OF UINT;
  END_VAR
  VAR_OUTPUT
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_WRITE_MULTIPLE_REGISTERS(Address, Index, Inputs, State) */

      MB_RTU_WRITE_MULTIPLE_REGISTERS(1, 0, IN, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_write_single_coil
  VAR_INPUT
    IN : BOOL;
  END_VAR
  VAR_OUTPUT
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_WRITE_SINGLE_COIL(Address, Index, Input, State) */

      MB_RTU_WRITE_SINGLE_COIL(1, 0, IN, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_write_multiple_coils
  VAR_INPUT
    IN : ARRAY [0..7] OF BOOL;
  END_VAR
  VAR_OUTPUT
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_WRITE_MULTIPLE_COILS(Address, Index, Inputs, State) */

      MB_RTU_WRITE_MULTIPLE_COILS(1, 0, IN, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_read_holding_registers
  VAR_OUTPUT
    OUT : ARRAY [0..7] OF UINT;
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_READ_HOLDING_REGISTERS(Address, Index, Outputs, State) */

      MB_RTU_READ_HOLDING_REGISTERS(1, 0, OUT, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_read_coils
  VAR_OUTPUT
    OUT : ARRAY [0..7] OF BOOL;
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_READ_COILS(Address, Index, Outputs, State) */

      MB_RTU_READ_COILS(1, 0, OUT, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_read_input_registers
  VAR_OUTPUT
    OUT : ARRAY [0..7] OF UINT;
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_READ_INPUT_REGISTERS(Address, Index, Outputs, State) */

      MB_RTU_READ_INPUT_REGISTERS(1, 0, OUT, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_write_single_register
  VAR_INPUT
    IN : UINT;
  END_VAR
  VAR_OUTPUT
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_WRITE_SINGLE_REGISTER(Address, Index, Input, State) */

      MB_RTU_WRITE_SINGLE_REGISTER(1, 0, IN, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

FUNCTION_BLOCK MB_read_discrete_inputs
  VAR_OUTPUT
    OUT : ARRAY [0..7] OF BOOL;
    Q : BOOL;
  END_VAR

  {
  #ifdef MODBUS_MASTER
  #include "modbus_master.h"

      /* MB_RTU_READ_DISCRETE_INPUTS(Address, Index, Outputs, State) */

      MB_RTU_READ_DISCRETE_INPUTS(1, 0, OUT, Q);

  #else

      /* return not OK for simulation */
      SetFbVar(Q, 0);

  #endif
  }
END_FUNCTION_BLOCK

PROGRAM ModbusMaster
  VAR
    OUTW : ARRAY [0..7] OF UINT;
    INW1 : ARRAY [0..7] OF UINT;
    OUTX : ARRAY [0..7] OF BOOL;
    INX1 : ARRAY [0..7] OF BOOL;
    Q : BOOL;
    INX0 : BOOL;
    INW0 : UINT;
    read_coils : MB_read_coils;
    read_discrete_inputs : MB_read_discrete_inputs;
    read_holding_registers : MB_read_holding_registers;
    read_input_registers : MB_read_input_registers;
    write_single_coil : MB_write_single_coil;
    write_multiple_coils : MB_write_multiple_coils;
    write_single_register : MB_write_single_register;
    write_multiple_registers : MB_write_multiple_registers;
  END_VAR
  VAR
    inp0 AT %IW0 : UINT;
    inp1 AT %IW1 : UINT;
    inp2 AT %IW2 : UINT;
    inp3 AT %IW3 : UINT;
    inp4 AT %IW4 : UINT;
    inp5 AT %IW5 : UINT;
  END_VAR

  read_coils(
       OUT => OUTX,
       Q => Q);

  read_discrete_inputs(
       OUT => OUTX,
       Q => Q);

  read_holding_registers(
       OUT => OUTW,
       Q => Q);

  read_input_registers(
       OUT => OUTW,
       Q => Q);

  write_single_coil(
       IN := INX0,
       Q => Q);

  write_multiple_coils(
       IN := INX1,
       Q => Q);

  write_single_register(
       IN := INW0,
       Q => Q);

  INW1[0] := inp0;
  INW1[1] := inp1;
  INW1[2] := inp2;
  INW1[3] := inp3;
  INW1[4] := inp4;
  INW1[5] := inp5;

  write_multiple_registers(
       IN := INW1,
       Q => Q);
END_PROGRAM


CONFIGURATION config

  RESOURCE resource1 ON PLC
    TASK task0(INTERVAL := T#20ms,PRIORITY := 0);
    PROGRAM instance0 WITH task0 : ModbusMaster;
  END_RESOURCE
END_CONFIGURATION
