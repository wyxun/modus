#ifndef __MODUS_MWAVEFORM_H__
#define __MODUS_MWAVEFORM_H__

#include <stdint.h>
#include <stdbool.h>
#include "global_define.h"
#include "mwaveform_protocol.h"

#ifdef MODUS_CFG_USER_CONFIG_INCLUSION
#   include MODUS_CFG_USER_CONFIG_INCLUSION
#endif

/*============================ CONFIGURATION =================================*/

#ifndef MWAVEFORM_ENABLE
#   define MWAVEFORM_ENABLE             0
#endif

#ifndef MWAVEFORM_MAX_CHANNELS
#   define MWAVEFORM_MAX_CHANNELS       16
#endif

#ifndef MWAVEFORM_RTT_BUFFER_SIZE
#   define MWAVEFORM_RTT_BUFFER_SIZE    1024        /* Default RTT buffer size */
#endif

#ifndef MWAVEFORM_FIFO_DEPTH
#   define MWAVEFORM_FIFO_DEPTH         16          /* Default FIFO depth */
#endif


/* Max data frame size: SYNC(2)+SEQ(1)+MASK+DATA+CRC(1) */
#define MWAVEFORM_FRAME_SIZE                                                    \
    (3 + ((MWAVEFORM_MAX_CHANNELS + 7) / 8) + 2 * MWAVEFORM_MAX_CHANNELS + 1)

#ifndef MWAVEFORM_RTT_CHANNEL
#   define MWAVEFORM_RTT_CHANNEL        1
#endif

#ifndef MWAVEFORM_DECIMATION
#   define MWAVEFORM_DECIMATION         1
#endif

#ifndef MWAVEFORM_BATCH_ENABLE
#   define MWAVEFORM_BATCH_ENABLE       1
#endif

#ifndef MWAVEFORM_BATCH_SIZE
#   define MWAVEFORM_BATCH_SIZE         64
#endif

#ifndef MWAVEFORM_BATCH_DEPTH
#   define MWAVEFORM_BATCH_DEPTH        128
#endif

#ifndef MWAVEFORM_BATCH_FLUSH_MS
#   define MWAVEFORM_BATCH_FLUSH_MS     10
#endif

#ifndef MWAVEFORM_DEFAULT_SAMPLE_PERIOD_NS
#   define MWAVEFORM_DEFAULT_SAMPLE_PERIOD_NS  1000000u
#endif

#define MWAVEFORM_VAR_NONE                  0u
#define MWAVEFORM_VAR_FLOAT                 1u
#define MWAVEFORM_VAR_RAW                   2u

#define MWAVEFORM_MASK_BYTES                    \
    ((MWAVEFORM_MAX_CHANNELS + 7) / 8)

#define MWAVEFORM_BATCH_MAX_FRAME_SIZE          \
    (17 + MWAVEFORM_BATCH_SIZE *                \
         (MWAVEFORM_MASK_BYTES + 2 * MWAVEFORM_MAX_CHANNELS))

/*============================ MACROS ========================================*/

#if MWAVEFORM_ENABLE

/*============================ TYPES =========================================*/

typedef struct {
    int      (*Init)(const mwaveform_protocol_t *ptProtocol);
    uint8_t  (*AddChannel)(const char *pchName, float fScale);
    uint8_t  (*AddVariable)(const char *pchName, float fScale,
                            void *pvValue, uint8_t chType);
    void     (*Start)(void);
    void     (*Stop)(void);
    void     (*Push)(uint8_t chID, float fValue);
    void     (*PushRaw)(uint8_t chID, int16_t hwValue);
    void     (*Step)(void);
    void     (*Poll)(void);
    void     (*SetRate)(uint32_t wDecimation);  /* 0=external drive, n=every n-th Step call sends */
    void     (*SetDecimation)(uint8_t decimation);
    uint32_t (*SetStreamRate)(uint32_t isrPeriodNs, uint32_t targetHz);
    uint32_t (*SetChannelRate)(uint8_t chID, uint32_t hz);
    void     (*SetSamplePeriodNs)(uint32_t periodNs);
    uint32_t (*GetDropCount)(void);             /* Total dropped frames */
    uint32_t (*GetLastIntervalDrops)(void);     /* Drops in last 1s */
    uint32_t (*GetRTTFullCount)(void);          /* RTT congestion events */
    void     (*ClearDropCount)(void);

} mwaveform_api_t;

extern const mwaveform_api_t mwaveform;

/*============================ PROTOTYPES ====================================*/

#else /* MWAVEFORM_ENABLE == 0 */

typedef struct {
    int      (*Init)(const mwaveform_protocol_t *ptProtocol);
    uint8_t  (*AddChannel)(const char *pchName, float fScale);
    uint8_t  (*AddVariable)(const char *pchName, float fScale,
                            void *pvValue, uint8_t chType);
    void     (*Start)(void);
    void     (*Stop)(void);
    void     (*Push)(uint8_t chID, float fValue);
    void     (*PushRaw)(uint8_t chID, int16_t hwValue);
    void     (*Step)(void);
    void     (*Poll)(void);
    void     (*SetRate)(uint32_t wDecimation);
    void     (*SetDecimation)(uint8_t decimation);
    uint32_t (*SetStreamRate)(uint32_t isrPeriodNs, uint32_t targetHz);
    uint32_t (*SetChannelRate)(uint8_t chID, uint32_t hz);
    void     (*SetSamplePeriodNs)(uint32_t periodNs);
    uint32_t (*GetDropCount)(void);
    uint32_t (*GetLastIntervalDrops)(void);
    uint32_t (*GetRTTFullCount)(void);
    void     (*ClearDropCount)(void);
} mwaveform_api_t;

extern const mwaveform_api_t mwaveform;

#endif /* MWAVEFORM_ENABLE */

#endif /* __MODUS_MWAVEFORM_H__ */
