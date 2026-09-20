#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "trace.h"
#include "util_debug.h"

#define ABS(__N)    ((__N) < 0 ? -(__N) : (__N))
#define _BV(__N)    ((uint32_t)1<<(__N))
#define TOP         (0x0FFF)

/* 运行期日志掩码，初始值由 MLOG_MASK_DEFAULT 决定（默认全开 0x1F）
 * 可通过 mshell log 命令动态修改，或在 userconfig.h 中覆盖 MLOG_MASK_DEFAULT */
uint8_t g_chGLogMask = MLOG_MASK_DEFAULT;

void (*pfcnLedSet)(bool bStatus);

void util_debug_Printf(const char *format, ...)
{
    if (NULL == format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    const char *pchStart = format;
    const char *p = format;
    
    while (*p) {
        if (*p == '%') {
            /* Output prior segment */
            if (p > pchStart) {
                TRACE.ToString.Buffer(pchStart, (uint_fast16_t)(p - pchStart));
            }
            
            p++;
            if (*p == '\0') break;
            
            /* Skip flags and width (e.g. '08' in '%08x') */
            while ((*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-' || *p == '#') {
                p++;
                if (*p == '\0') break;
            }
            if (*p == '\0') break;
            
            /* Handle length modifiers (e.g. 'l' in '%lu') */
            if (*p == 'l') {
                p++;
                if (*p == 'l') p++; /* skip 'll' as well */
            }
            if (*p == '\0') break;
            
            /* Handle format specifier */
            switch (*p) {
                case 'd':
                case 'i':
                    TRACE.ToString.Int32(va_arg(args, int32_t));
                    break;
                case 'u':
                    /* Map %u to Int32 since it handles positive numbers well in decimal */
                    TRACE.ToString.Int32((int32_t)va_arg(args, uint32_t));
                    break;
                case 'x':
                case 'X':
                    TRACE.ToString.UInt32(va_arg(args, uint32_t));
                    break;
                case 'p':
                    TRACE.ToString.UInt32((uintptr_t)va_arg(args, void *));
                    break;
                case 's':
                    TRACE.ToString.String(va_arg(args, const char *));
                    break;
                case 'c': {
                    char ch = (char)va_arg(args, int);
                    TRACE.ToString.Buffer(&ch, 1);
                    break;
                }
                case 'f':
                    TRACE.ToString.Float(
                        (float)va_arg(args, double)
                    );
                    break;
                case '%':
                    TRACE.ToString.Buffer("%", 1);
                    break;
                default:
                    /* Unrecognized: print as is */
                    TRACE.ToString.Buffer(p - 1, 2);
                    break;
            }
            pchStart = p + 1;
        }
        p++;
    }
    
    /* Output remaining tail */
    if (p > pchStart) {
        TRACE.ToString.Buffer(pchStart, (uint_fast16_t)(p - pchStart));
    }
    
    va_end(args);
}

/*! \brief set the 16-level led gradation
 *! \param hwLevel gradation
 *! \return none
 */
static void set_led_gradation(uint16_t hwLevel)
{
    static uint16_t s_hwCounter = 0;
    
    if (hwLevel >= s_hwCounter) {
        pfcnLedSet(0);
    } else {
        pfcnLedSet(1);
    }
    
    s_hwCounter++;
    s_hwCounter &= TOP;
}

int utildebug_LedBreathe(void *ptVoid) 
{
    static uint16_t s_hwCounter = 0;
    static int16_t s_nGray = (TOP >> 1);
    
    s_hwCounter++;
    if (!(s_hwCounter & (_BV(11)-1))) {
        s_nGray++; 
        if (s_nGray == TOP) {
            s_nGray = 0;
        }
    }
    
    set_led_gradation(ABS(s_nGray - (TOP >> 1)));
    return 0;
}

void utildebug_LedInit(void (*fcnLedSet)(bool bStatus))
{
    pfcnLedSet = fcnLedSet;
}


