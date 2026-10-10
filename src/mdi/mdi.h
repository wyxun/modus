/**
 * @file mdi.h
 * @brief Public compile-time MODUS hardware interface.
 * @author Codex
 * @date 2026-09-19
 * @note Include this header for chip-independent core contracts. Optional
 *       features require their own headers. Include one chip/board instance
 *       to supply concrete resources such as `bridge`.
 *       No runtime device object or function table is created by this header.
 */
#ifndef MODUS_MDI_H
#define MODUS_MDI_H

/* Optional software compositions are included explicitly by their users. */
#include "mdi/core/contract.h"
#include "mdi/core/timer.h"
#include "mdi/core/tick.h"
#include "mdi/core/stream.h"
#include "mdi/core/bind.h"
#include "mdi/core/adc.h"

#endif
