#include <stdlib.h>
extern "C" {
#include "openplc.h"
}
#include "Arduino.h"
#include "hw.h"

uint8_t pinMask_DIN[] = DIN;
uint8_t pinMask_AIN[] = AIN;
uint8_t pinMask_DOUT[] = DOUT;
uint8_t pinMask_AOUT[] = AOUT;

void hardwareInit()
{
    for (uint8_t i = 0; i < NUM(pinMask_DIN); i++)
        pinMode(pinMask_DIN[i], INPUT);

    for (uint8_t i = 0; i < NUM(pinMask_AIN); i++)
        pinMode(pinMask_AIN[i], INPUT);

    for (uint8_t i = 0; i < NUM(pinMask_DOUT); i++)
        pinMode(pinMask_DOUT[i], OUTPUT);

    for (uint8_t i = 0; i < NUM(pinMask_AOUT); i++)
        pinMode(pinMask_AOUT[i], OUTPUT);
}

void updateInputBuffers()
{
    for (uint8_t i = 0; i < NUM(pinMask_DIN); i++)
        IX[i] = digitalRead(pinMask_DIN[i]);

    for (uint8_t i = 0; i < NUM(pinMask_AIN); i++)
        IW[i] = (analogRead(pinMask_AIN[i]) * 64);
}

void updateOutputBuffers()
{
    for (uint8_t i = 0; i < NUM(pinMask_DOUT); i++)
        digitalWrite(pinMask_DOUT[i], QX[i]);

    for (uint8_t i = 0; i < NUM(pinMask_AOUT); i++)
        analogWrite(pinMask_AOUT[i], QW[i] / 256);
}
