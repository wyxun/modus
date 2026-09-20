#include <stdint.h>
#include "modus.h"
#if MSHELL_ENABLE
#   include "mdebug/mshell.h"
#endif
#if MWAVEFORM_ENABLE
#   include "mdebug/mwaveform.h"
#endif

/* mstorage 可选模块附着点：
 * 当 mstorage.c 未加入编译时，此 weak 定义生效 —— no-op。
 * 当 mstorage.c 已加入编译时，其内部的强定义自动覆盖此 stub。 */
__attribute__((weak)) void mstorage_SetDefaultFlash(void *ptFlash)
{
    (void)ptFlash; /* mstorage not included in this build */
}

/* Board projects override these no-op hooks from their peripheral/mdi layer.
 * Keeping the weak defaults here lets generic MODUS targets link unchanged. */
__attribute__((weak)) void mdi_Service(void)
{
}

__attribute__((weak)) void mdi_Clock(void)
{
}

#ifdef LINUX_POSIX
#include <stdio.h>
#endif

// MODUS version static structure is defined using macros from global_define.h
const struct {
    uint8_t chPurpose;          //!< software framework purpose
    uint8_t chInterface;        //!< interface version
    uint8_t chMajor;            //!< major version
    uint8_t chMinor;            //!< minor version
} MODUSVersion = MODUS_VERSION;

static bool s_bModusInit = false;

/* GNU ld style (weak) */
extern const modus_init_info_t __start_init_infos[] __attribute__((weak));
extern const modus_init_info_t __stop_init_infos[]  __attribute__((weak));

#ifdef __ARMCC_VERSION
/* Keil/armlink style symbols (objects) */
extern const uint8_t Image$$init_infos$$Base;
extern const uint8_t Image$$init_infos$$Limit;
#endif

/**
 * Function: modus_Init
 * ----------------------------
 * This function initializes the MODUS framework. It initializes the MODUS storage and coroutine, 
 * and prints the MODUS version.
 *
 * Parameters: 
 * ptModus: A pointer to the MODUS structure.
 *
 * Returns: 
 * None
 */
void modus_Init(modus_t *ptModus)
{
    const modus_init_info_t *start = NULL;
    const modus_init_info_t *stop  = NULL;

    // Check if the input parameter is NULL
    if (ptModus == NULL) {
        MLOG(E, "Error: ptModus is NULL.\n");
        return;
    }

    MLOG(I, "MODUS VERSION :");
    MLOG(I, (uint8_t *)&MODUSVersion, 4);
    MLOG(I, "\n");

    /* 将调用方提供的默认 Flash 注入 mstorage 层，
     * 使 ptFlash == NULL 的 mstorage 实例在 Init 时自动绑定 */
    mstorage_SetDefaultFlash(ptModus->ptAppFlash);


if ((const void *)__start_init_infos != NULL && (const void *)__stop_init_infos != NULL &&
    __start_init_infos < __stop_init_infos) {
    start = __start_init_infos;
    stop  = __stop_init_infos;
}
#ifdef __ARMCC_VERSION
else {
    /* armlink provides Image$$<sec>$$Base/Limit as addresses — take address & cast */
    const modus_init_info_t *arm_start = (const modus_init_info_t *)&Image$$init_infos$$Base;
    const modus_init_info_t *arm_stop  = (const modus_init_info_t *)&Image$$init_infos$$Limit;
    if ((const void *)arm_start != NULL && (const void *)arm_stop != NULL && arm_start < arm_stop) {
        start = arm_start;
        stop  = arm_stop;
    }
}
#endif

    if (start != NULL && stop != NULL && start < stop) {
        for (const modus_init_info_t *info = start; info < stop; ++info) {
            if (info->pfcnInitFunc) {
                info->pfcnInitFunc(info->wObjectAddr, info->wConfigAddr);
            }
        }
    }

    // Initialize coroutine
    mcoroutine_Init();
    // Print list information
    mbase_DebugListBase();
    /* Mark framework as initialized so modus_Clock will execute callbacks */
    s_bModusInit = true;
}

/**
 * Function: modus_Run
 * ----------------------------
 * This function runs the MODUS framework. It traverses the list of objects, and
 * for each object, it calls the object's Run function.
 *
 * Parameters: 
 * None
 *
 * Returns: 
 * None
 */
void modus_Run(void)
{
    // read point only
    mlist_t *const ptListObject = mbase_GetBaseList();

    // Check if the list object is NULL
    if (ptListObject == NULL) {
        MLOG(E, "Error: ptListObject is NULL.\n");
        return;
    }

    mdi_Service();

    // read value only
    const mlist_item_t *ptListItemDes;
    modus_base_t *ptBaseDes;

    for (ptListItemDes = ptListObject->tListEnd.ptPrevious;                     \
                ptListItemDes != &ptListObject->tListEnd;                       \
            ptListItemDes = ptListItemDes->ptPrevious) {
        ptBaseDes = ptListItemDes->pvOwner;

        // Check if the base descriptor is NULL
        if (NULL == ptBaseDes) {
            MLOG(E, "Error: ptBaseDes is NULL.\n");
            return;
        }

        // Check if the function interface is NULL
        if (NULL == ptBaseDes->pFcnInterface) {
            MLOG(E, "Error: ptBaseDes->pFcnInterface is NULL.\n");
            return;
        }

        ptBaseDes->pFcnInterface->Run(ptBaseDes->wParent);
    }

    mcoroutine_Run();
#if MSHELL_ENABLE
    mshell_Poll();          /* 调试 shell 轮询 */
#endif
#if MWAVEFORM_ENABLE
    mwaveform.Poll();       /* 波形采集轮询 */
#endif
}



/**
 * Function: modus_Clock
 * ----------------------------
 * This function calls the Clock function of each object in the MODUS framework. 
 * It traverses the list of objects, and for each object, it calls the object's 
 * Clock function.
 * 
 * Parameters: 
 * None
 *
 * Returns: 
 * None
 */
void modus_Clock(void)
{
    // read point only
    mlist_t *const ptListObject = mbase_GetBaseList();
    
    if(false == s_bModusInit)
        return;
    
    // Check if the list object is NULL
    if (NULL == ptListObject) {
        MLOG(E, "Error: ptListObject is NULL.\n");
        return;
    }

    mdi_Clock();

    // read value only
    const mlist_item_t *ptListItemDes;
    modus_base_t *ptBaseDes;

    for (ptListItemDes = ptListObject->tListEnd.ptPrevious;                     \
            ptListItemDes != &ptListObject->tListEnd;                           \
            ptListItemDes = ptListItemDes->ptPrevious) {
        ptBaseDes = ptListItemDes->pvOwner;

        // Check if the base descriptor is NULL
        if (NULL == ptBaseDes) {
            MLOG(E, "Error: ptBaseDes is NULL.\n");
            return;
        }

        // Check if the function interface is NULL
        if (ptBaseDes->pFcnInterface == NULL) {
            MLOG(E, "Error: ptBaseDes->pFcnInterface is NULL.\n");
            return;
        }

        ptBaseDes->pFcnInterface->Clock(ptBaseDes->wParent);
    }
    
#if MWAVEFORM_ENABLE
    extern void mwaveform_Default_Step_Callback(void);
    mwaveform_Default_Step_Callback();
#endif
}

/**
 * Function: assert_failed
 * ----------------------------
 * This function is called when an assertion fails. It prints an error message, 
 * including the file where the assertion failed and the line number of the failure.
 *
 * Parameters: 
 * file: The file where the assertion failed.
 * line: The line number of the assertion failure.
 *
 * Returns: 
 * None. This function enters an infinite loop after printing the error message.
 */
void assert_failed(char *file, uint32_t line)
{
    MLOGF(E, "assert failed--> %s on line: %u\n", file, (unsigned)line);
    while(1);
}
