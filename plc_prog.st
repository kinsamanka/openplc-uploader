PROGRAM program0
  VAR
    and_a AT %QX0.0 : BOOL;
    and_b AT %QX0.1 : BOOL;
    and_c AT %QX0.2 : BOOL;
    and_out AT %QX0.3 : BOOL;
  END_VAR

  and_out := AND(and_a, and_b, and_c);
END_PROGRAM


CONFIGURATION Config0

  RESOURCE Res0 ON PLC
    TASK task0(INTERVAL := T#20ms,PRIORITY := 0);
    PROGRAM instance0 WITH task0 : program0;
  END_RESOURCE
END_CONFIGURATION
