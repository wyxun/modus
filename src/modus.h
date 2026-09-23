#ifndef __MODUS_H__
#define __MODUS_H__

// Include necessary libraries
#include "mbase.h"
#include "global_define.h"
#include "mdebug/util_debug.h"
#include "mcoroutine.h"
#include "mlog.h"
/* NOTE: mstorage.h is intentionally NOT included here.
 * Users who declare mstorage objects must include mstorage.h themselves. */

#undef this
#define this    (*ptThis)

typedef struct {
    /**
     * @brief 默认 Flash 设备，供 mstorage 使用（可选）。
     *
     * 赋值 HW.ptAppFlash 后 modus_Init 内部自动绑定到所有 mstorage 实例。
     * 若不使用 mstorage，保持 NULL 即可。
     * 使用 void* 以避免对 mdi_flash_t 类型的强依赖——
     * mstorage.c 内部负责转型。
     */
    void *ptAppFlash;
} modus_t;

#define MODUS_DECLARE_OBJECT(object_type, object_name, ...)                     \
    object_type##_cfg_t t##object_name##Cfg = { __VA_ARGS__ };                  \
    object_type##_t t##object_name;                                             \
    INIT_SECTION const modus_init_info_t init_info_##object_name = {            \
        .pfcnInitFunc = (init_func_t)object_type##_Init,                        \
        .wObjectAddr = (uintptr_t)&t##object_name,                              \
        .wConfigAddr = (uintptr_t)&t##object_name##Cfg                          \
    };

// Function prototypes
void modus_Init(modus_t *ptModus);
void modus_Clock(void);
void modus_Run(void);

/* Board-level MDI hooks. The core only defines the call boundary; each target
 * may provide strong implementations. */
void mdi_Init(void);
void mdi_Service(void);
void mdi_Clock(void);

#endif  // __MODUS_H__
