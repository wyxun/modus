/**
 * @file mdi.h
 * @brief Public compile-time MODUS hardware interface.
 * @author Codex
 * @date 2026-09-19
 * @note Include this header for the chip-independent contract and optional
 *       feature APIs. Include one chip/board instance after this file; the
 *       instance supplies resources such as `bridge` or `phase_current`.
 *       No runtime device object or function table is created by this header.
 */
#ifndef MODUS_MDI_H
#define MODUS_MDI_H

/* Public layers: core contracts first, then optional compositions. */
#include "mdi/core/contract.h"
#include "mdi/core/timer.h"
#include "mdi/core/tick.h"
#include "mdi/core/stream.h"
#include "mdi/core/bind.h"
#include "mdi/core/adc.h"
#include "mdi/feature/adc_dma.h"
#include "mdi/feature/adc_mean.h"
#include "mdi/feature/foc.h"
#include "mdi/feature/i2c_reg8.h"
#include "mdi/feature/soft_i2c_edges.h"
#include "mdi/feature/soft_i2c_master.h"
#include "mdi/feature/spi_eeprom_25xx.h"

#endif
