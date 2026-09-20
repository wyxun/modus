/**
 * @file timer.h
 * @brief Static MDI contract for configurable peripheral timers.
 */
#ifndef MDI_CORE_TIMER_H
#define MDI_CORE_TIMER_H

#include "mdi/core/contract.h"

/* Timer resources are compile-time tokens.  The provider owns clock selection,
 * period calculation, update-event routing and hardware error mapping. */
#define MDI_TIMER_SetFrequency(R, HZ) MDI_OP(R, _timer_SetFrequency)((HZ))
#define MDI_TIMER_Start(R)             MDI_OP(R, _timer_Start)()
#define MDI_TIMER_Stop(R)              MDI_OP(R, _timer_Stop)()
#define MDI_TIMER_IsRunning(R)         MDI_OP(R, _timer_IsRunning)()

#endif /* MDI_CORE_TIMER_H */
