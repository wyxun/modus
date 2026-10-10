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

/* Keep the baseline in the caller. For deadlines use perf_counter with an
 * explicit caller-owned timeout slot; raw subtraction uses mdi_tick_t. */

#endif /* MDI_CORE_TICK_H */
