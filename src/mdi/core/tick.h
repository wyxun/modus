/**
 * @file tick.h
 * @brief Static MDI contract for an unscaled raw tick source.
 */
#ifndef MDI_CORE_TICK_H
#define MDI_CORE_TICK_H

#include "mdi/core/contract.h"

/* perf_counter exposes a signed 64-bit cycle count.  MDI treats the value as
 * an unsigned monotonic modulo counter so subtraction remains wrap-safe. */
typedef uint64_t mdi_tick_t;

#define MDI_TICK_Now(R) ((mdi_tick_t)MDI_OP(R, _tick_Now)())

/* The stateful convenience form intentionally owns one baseline per macro
 * expansion site.  This is a GNU C extension supported by the GCC/Clang
 * toolchains used by the board projects; callers that need portable or
 * independently mutable state should keep their own old tick and subtract. */
#if defined(__GNUC__) || defined(__clang__)
#define MDI_TICK_Elapsed(R, LIMIT)                                             \
    __extension__ ({                                                           \
        static mdi_tick_t s_wOld;                                             \
        static bool s_bStarted;                                               \
        mdi_tick_t wTickNow = MDI_TICK_Now(R);                                 \
        mdi_tick_t wTickLimit = (mdi_tick_t)(LIMIT);                            \
        bool bElapsed = s_bStarted                                             \
                     && ((mdi_tick_t)(wTickNow - s_wOld) >= wTickLimit);       \
        if (!s_bStarted || bElapsed) {                                         \
            s_wOld = wTickNow;                                                 \
            s_bStarted = true;                                                 \
        }                                                                      \
        bElapsed;                                                              \
    })
#endif

#endif /* MDI_CORE_TICK_H */
