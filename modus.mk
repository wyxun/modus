# MODUS — Bare-metal Embedded Framework Build Include
#
# Usage in external project Makefile:
#   MODUS_ROOT ?= lib/modus
#   include $(MODUS_ROOT)/modus.mk
#
#   # Opt-in to needed modules (all default 0)
#   MBLINFO_ENABLE   = 1
#   MSTORAGE_ENABLE  = 1
#
#   C_SOURCES += $(MODUS_SRCS)
#   C_INCLUDES += $(MODUS_INCLUDES)
#   CFLAGS += $(MODUS_CFLAGS)

# ---------------------------------------------------------------------------
# Enable/disable switches — conservative defaults (all OFF)
# ---------------------------------------------------------------------------

# mshell + trace + SEGGER_RTT
MSHELL_ENABLE    ?= 0

# mwaveform + mwaveform_protocol
MWAVEFORM_ENABLE ?= 0

# mstorage
MSTORAGE_ENABLE  ?= 0

# mblinfo
MBLINFO_ENABLE   ?= 0

# MLOG / MLOGF macros
MODUS_USE_LOG     ?= 0

# MODUS_ASSERT macro
MODUS_USE_ASSERT  ?= 0

# ---------------------------------------------------------------------------
# Size-tuning overrides (defaults; override with ?= or = in your Makefile)
# ---------------------------------------------------------------------------
MWAVEFORM_MAX_CHANNELS      ?= 16
MWAVEFORM_RTT_BUFFER_SIZE   ?= 1024
MWAVEFORM_FIFO_DEPTH        ?= 16
MWAVEFORM_DECIMATION        ?= 1
MWAVEFORM_BATCH_ENABLE      ?= 1
MWAVEFORM_BATCH_SIZE        ?= 64
MWAVEFORM_BATCH_DEPTH       ?= 128
MWAVEFORM_BATCH_FLUSH_MS    ?= 10
MLOG_MASK_DEFAULT            ?= 0x1F

# ---------------------------------------------------------------------------
# MODUS_ROOT — path to this file's directory (auto-detected if not set)
# ---------------------------------------------------------------------------
MODUS_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# ---------------------------------------------------------------------------
# Source files — core (always compiled)
# ---------------------------------------------------------------------------
MODUS_SRCS_CORE = \
    $(MODUS_ROOT)/src/modus.c \
    $(MODUS_ROOT)/src/mbase.c \
    $(MODUS_ROOT)/src/mcoroutine.c \
    $(MODUS_ROOT)/src/mlog.c \
    $(MODUS_ROOT)/src/utilities/mlist.c \
    $(MODUS_ROOT)/src/utilities/mringbuf.c

# ---------------------------------------------------------------------------
# 默认启用 MODUS 内置的 perf_counter 移植
# 若外部工程已自定义移植（如 posix_uart/template），需在 include 本文件前设置
# MODUS_USE_DEFAULT_PERFC_PORT = 0
MODUS_USE_DEFAULT_PERFC_PORT ?= 1

# perf_counter 移植 + RISC-V bare-metal shim（独立于 debug 模块）
MODUS_SRCS_ARCH =

ifeq ($(MODUS_USE_DEFAULT_PERFC_PORT),1)
MODUS_SRCS_ARCH += \
    $(MODUS_ROOT)/src/arch/perfc_port.c \
    $(MODUS_ROOT)/src/arch/riscv/riscv_shim.c
endif

# Source files — debug (mshell + trace + SEGGER_RTT)
# ---------------------------------------------------------------------------
MODUS_SRCS_DEBUG = \
    $(MODUS_ROOT)/src/mdebug/mshell.c \
    $(MODUS_ROOT)/src/mdebug/trace.c \
    $(MODUS_ROOT)/src/mdebug/trace_fmt.c \
    $(MODUS_ROOT)/src/mdebug/util_debug.c \
    $(MODUS_ROOT)/src/mdebug/segger_rtt/SEGGER_RTT.c

MODUS_SRCS_ARCH += \
    $(MODUS_ROOT)/src/arch/debug_transport.c

# 架构调试模块 (mshell 命令 + fault handler)，依赖 MSHELL_ENABLE
MODUS_SRCS_ARCH_DEBUG = \
    $(MODUS_ROOT)/src/arch/riscv/mdebug_riscv.c \
    $(MODUS_ROOT)/src/arch/cortex-m/mdebug_cm.c \
    $(MODUS_ROOT)/src/arch/cortex-m/fault_cm.c

MODUS_SRCS_DEBUG += $(MODUS_SRCS_ARCH_DEBUG)

# ---------------------------------------------------------------------------
# Source files — waveform
# ---------------------------------------------------------------------------
MODUS_SRCS_WAVEFORM = \
    $(MODUS_ROOT)/src/mdebug/mwaveform.c \
    $(MODUS_ROOT)/src/mdebug/mwaveform_protocol.c

# ---------------------------------------------------------------------------
# Source files — storage / blinfo
# ---------------------------------------------------------------------------
MODUS_SRCS_STORAGE = \
    $(MODUS_ROOT)/src/mstorage.c

MODUS_SRCS_BLINFO = \
    $(MODUS_ROOT)/src/mblinfo.c

# ---------------------------------------------------------------------------
# Aggregate source list by switches
# ---------------------------------------------------------------------------
MODUS_SRCS = $(MODUS_SRCS_CORE) $(MODUS_SRCS_ARCH)

ifneq ($(MSHELL_ENABLE),0)
MODUS_SRCS += $(MODUS_SRCS_DEBUG)
endif

ifneq ($(MWAVEFORM_ENABLE),0)
MODUS_SRCS += $(MODUS_SRCS_WAVEFORM)
endif

ifneq ($(MSTORAGE_ENABLE),0)
MODUS_SRCS += $(MODUS_SRCS_STORAGE)
endif

ifneq ($(MBLINFO_ENABLE),0)
MODUS_SRCS += $(MODUS_SRCS_BLINFO)
endif

# ---------------------------------------------------------------------------
# Include paths (fixed)
# ---------------------------------------------------------------------------
MODUS_INCLUDES = \
    -I$(MODUS_ROOT) \
    -I$(MODUS_ROOT)/src \
    -I$(MODUS_ROOT)/src/mdi \
    -I$(MODUS_ROOT)/src/arch \
    -I$(MODUS_ROOT)/src/arch/cortex-m \
    -I$(MODUS_ROOT)/src/arch/riscv

# ---------------------------------------------------------------------------
# Compile flags generated from switches
# ---------------------------------------------------------------------------

# Module enable flags (always emit as 0 or 1)
ifneq ($(MSHELL_ENABLE),0)
MODUS_CFLAGS += -DMSHELL_ENABLE=1
else
MODUS_CFLAGS += -DMSHELL_ENABLE=0
endif

ifneq ($(MWAVEFORM_ENABLE),0)
MODUS_CFLAGS += -DMWAVEFORM_ENABLE=1
else
MODUS_CFLAGS += -DMWAVEFORM_ENABLE=0
endif

# Log / assert (0 = disabled via negative define)
ifeq ($(MODUS_USE_LOG),0)
MODUS_CFLAGS += -D__NO_USE_LOG__
endif
ifeq ($(MODUS_USE_ASSERT),0)
MODUS_CFLAGS += -D__NO_USE_ASSERT
endif

# Size overrides (only emit if changed from default)
ifneq ($(MWAVEFORM_MAX_CHANNELS),16)
MODUS_CFLAGS += -DMWAVEFORM_MAX_CHANNELS=$(MWAVEFORM_MAX_CHANNELS)
endif
ifneq ($(MWAVEFORM_RTT_BUFFER_SIZE),1024)
MODUS_CFLAGS += -DMWAVEFORM_RTT_BUFFER_SIZE=$(MWAVEFORM_RTT_BUFFER_SIZE)
endif
ifneq ($(MWAVEFORM_FIFO_DEPTH),16)
MODUS_CFLAGS += -DMWAVEFORM_FIFO_DEPTH=$(MWAVEFORM_FIFO_DEPTH)
endif
ifneq ($(MWAVEFORM_DECIMATION),1)
MODUS_CFLAGS += -DMWAVEFORM_DECIMATION=$(MWAVEFORM_DECIMATION)
endif
ifneq ($(MWAVEFORM_BATCH_ENABLE),1)
MODUS_CFLAGS += -DMWAVEFORM_BATCH_ENABLE=$(MWAVEFORM_BATCH_ENABLE)
endif
ifneq ($(MWAVEFORM_BATCH_SIZE),64)
MODUS_CFLAGS += -DMWAVEFORM_BATCH_SIZE=$(MWAVEFORM_BATCH_SIZE)
endif
ifneq ($(MWAVEFORM_BATCH_DEPTH),128)
MODUS_CFLAGS += -DMWAVEFORM_BATCH_DEPTH=$(MWAVEFORM_BATCH_DEPTH)
endif
ifneq ($(MWAVEFORM_BATCH_FLUSH_MS),10)
MODUS_CFLAGS += -DMWAVEFORM_BATCH_FLUSH_MS=$(MWAVEFORM_BATCH_FLUSH_MS)
endif
ifneq ($(MLOG_MASK_DEFAULT),0x1F)
MODUS_CFLAGS += -DMLOG_MASK_DEFAULT=$(MLOG_MASK_DEFAULT)
endif
