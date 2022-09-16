#ifndef openplc_h
#define openplc_h

#include <stdint.h>

#include "config.h"

/*********************/
/*  IEC Types defs   */
/*********************/

typedef uint8_t  IEC_BOOL;

typedef int8_t    IEC_SINT;
typedef int16_t   IEC_INT;
typedef int32_t   IEC_DINT;
typedef int64_t   IEC_LINT;

typedef uint8_t    IEC_USINT;
typedef uint16_t   IEC_UINT;
typedef uint32_t   IEC_UDINT;
typedef uint64_t   IEC_ULINT;

typedef uint8_t    IEC_BYTE;
typedef uint16_t   IEC_WORD;
typedef uint32_t   IEC_DWORD;
typedef uint64_t   IEC_LWORD;

typedef float    IEC_REAL;
typedef double   IEC_LREAL;

//MatIEC Compiler
void config_run__(unsigned long tick);
void config_init__(void);

//Common task timer
extern unsigned long long common_ticktime__;
#define DELAY_TIME      20

//glueVars.c
void updateTime();

//OpenPLC Buffers
extern IEC_UINT QW[];
extern IEC_UINT IW[];
extern IEC_BOOL QX[];
extern IEC_BOOL IX[];

//Hardware Layer
void hardwareInit();
void updateInputBuffers();
void updateOutputBuffers();
void setupCycleDelay(unsigned long long cycle_time);
void cycleDelay();
#endif
