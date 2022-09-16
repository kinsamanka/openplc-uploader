#include "config.h"
#include "iec_std_lib.h"

void updateTime(void);

#define BIT_POS(x, y)               (x*8 + y)
#define CHECK_BOUNDS(z, a, b, c)    enum { var##c = 1/(!!(a < b)) }

#define ALLOC_QX(type, name, x, y) \
    CHECK_BOUNDS("error: Address exceeded COIL COUNT", \
            BIT_POS(x, y), QX_COUNT, name); \
    IEC_##type* name = &QX[BIT_POS(x, y)];

#define ALLOC_IX(type, name, x, y) \
    CHECK_BOUNDS("error: Address exceeded INPUT COUNT", \
            BIT_POS(x, y), IX_COUNT, name); \
    IEC_##type* name = &IX[BIT_POS(x, y)];

#define ALLOC_IW(type, name, x) \
    CHECK_BOUNDS("error: Address exceeded INPUT REGISTER COUNT", \
            x, IW_COUNT, name); \
    IEC_##type* name = &IW[x];

#define ALLOC_QW(type, name, x) \
    CHECK_BOUNDS("error: Address exceeded HOLDING REGISTER COUNT", \
            x, QW_COUNT, name); \
    IEC_##type* name = &QW[x];

#define __LOCATED_VAR(type, name, pre, suf, loc, ...) \
    ALLOC_##pre ## suf(type,name,loc, ## __VA_ARGS__)

IEC_UINT QW[QW_COUNT];
IEC_UINT IW[IW_COUNT];
IEC_BOOL QX[QX_COUNT];
IEC_BOOL IX[IX_COUNT];

#include "generated/LOCATED_VARIABLES.h"

TIME __CURRENT_TIME;
BOOL __DEBUG;
extern unsigned long long common_ticktime__;

void updateTime(void)
{
    const TIME ticktime = {0, common_ticktime__};

    __CURRENT_TIME = __time_add(__CURRENT_TIME, ticktime);
}
