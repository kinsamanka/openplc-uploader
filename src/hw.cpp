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
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
        pinMode(pinMask_DIN[i], INPUT);

    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
        pinMode(pinMask_AIN[i], INPUT);

    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
        pinMode(pinMask_DOUT[i], OUTPUT);

    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
        pinMode(pinMask_AOUT[i], OUTPUT);
}

void updateInputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_INPUT; i++)
        IX[i] = digitalRead(pinMask_DIN[i]);

    for (int i = 0; i < NUM_ANALOG_INPUT; i++)
        IW[i] = (analogRead(pinMask_AIN[i]) * 64);
}

void updateOutputBuffers()
{
    for (int i = 0; i < NUM_DISCRETE_OUTPUT; i++)
        digitalWrite(pinMask_DOUT[i], QX[i]);

    for (int i = 0; i < NUM_ANALOG_OUTPUT; i++)
        analogWrite(pinMask_AOUT[i], QW[i] / 256);
}
