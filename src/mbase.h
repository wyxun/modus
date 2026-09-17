#ifndef __MODUS_BASE_H__
#define __MODUS_BASE_H__

// Include necessary libraries
#include "global_define.h"
#include "utilities/mlist.h"

// Define types
typedef struct {
    int (*Clock)(uintptr_t wObjectAddr);
    int (*Run)(uintptr_t wObjectAddr);
} modus_interface_t;

typedef struct{
    uint8_t *buffer;
    uint16_t hwReadIndex;
    uint16_t hwWriteIndex;
    uint16_t hwBufferSize;
} object_ring_buffer_t;

typedef struct {
    uint8_t *pchMessage;
    uint16_t hwLength;
    uint16_t hwMaxSize;
}message_t;

typedef struct {
    uint8_t *pchBuffer;
    uint16_t hwSize;
} share_mem_t;

typedef struct {
    uint8_t *pchMessage;
    uint16_t hwLength;
    uint16_t hwMaxSize;
    mlist_item_t tListItem;
} message_item_t;

typedef struct {
    uint32_t wId;
    uintptr_t wParent;
    share_mem_t *ptShareMem;
    
    /* 新增的 RingBuffer 框架自动绑定参数 */
    uint8_t  *pchRingBuffer;
    uint16_t  hwRingSize;
    
    modus_interface_t FcnInterface;
} modus_base_cfg_t;

typedef struct {
    uint32_t wId;
    uint32_t wEvent;
    mlist_t tListMessage;
    object_ring_buffer_t tRingBuffer;
    modus_interface_t *pFcnInterface;
    share_mem_t *ptShareMem;
    uintptr_t wParent;
    mlist_item_t tListItem;
} modus_base_t;

// Function prototypes
int mbase_Init(modus_base_t *ptBase, modus_base_cfg_t *ptCfg);
int mbase_EventPost(uint32_t wId, uint32_t wEvent);
uint32_t mbase_EventPend(modus_base_t *ptBase);
int mbase_MessagePost(uint32_t wId, message_item_t *ptMsgItem) __attribute__((deprecated("Use RingBuffer IPC instead")));
int mbase_MessagePend(modus_base_t *ptBase, message_t *ptMsg) __attribute__((deprecated("Use RingBuffer IPC instead")));
int mbase_MessagePostToRing(uint32_t wId, uint8_t *pchMsgBuffer, 
                            uint16_t hwLength);
int mbase_MessagePendFromRing(modus_base_t *ptBase, uint8_t *pchMsgBuffer, 
                              uint16_t hwMaxSize);
share_mem_t* mbase_ShareMemRead(uint32_t wId) __attribute__((deprecated("Use strong-typed DI pointer instead")));
mlist_t* mbase_GetBaseList(void);
void mbase_DebugListBase(void);

// msg item macros
#define MODUS_MSG_ITEM_DECLARE(OBJECT,NAME,SIZE)                                \
        uint8_t ch##NAME##_Buffer[SIZE] = {0};                                  \
        message_item_t t##NAME##item = {                                        \
            .tListItem.wItemValue = OBJECT,                                     \
            .pchMessage = ch##NAME##_Buffer,                                    \
            .hwLength = 0,                                                      \
            .hwMaxSize = SIZE                                                   \
        };
#define MODUS_MSG_ITEM_INITIALISE_LIST(NAME)                                    \
    do{                                                                         \
        mlist_ItemInit(&(t##NAME##item).tListItem);                             \
        (t##NAME##item).tListItem.wItemValue = (t##NAME##item).hwLength;        \
        (t##NAME##item).hwLength = 0;                                           \
        (t##NAME##item).tListItem.pvOwner = &(t##NAME##item);                   \
    }while(0)

#define MODUS_MSG_ITEM_GET_HANDLE(NAME) &(t##NAME##item)   

#define MODUS_MSG_ITEM_UPDATE(NAME, MESSAGE, LENGTH)                            \
    do{                                                                         \
        if((t##NAME##item).hwMaxSize >= LENGTH)                                 \
        {                                                                       \
            memcpy((t##NAME##item).pchMessage, MESSAGE, LENGTH);                \
            (t##NAME##item).hwLength = LENGTH;                                  \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            (t##NAME##item).hwLength = 0;                                       \
        }                                                                       \
    }while(0)

#define MODUS_MSG_ITEM_GET_BUFFER(NAME) (t##NAME##item).pchMessage
#define MODUS_MSG_ITEM_GET_LENGTH(NAME) (t##NAME##item).hwLength        

// msg macros
#define MODUS_MSG_DECLARE(NAME,SIZE)                                            \
        uint8_t ch##NAME##_Buffer[SIZE] = {0};                                  \
        message_t t##NAME##Msg = {                                              \
            .pchMessage = ch##NAME##_Buffer,                                    \
            .hwLength = 0,                                                      \
            .hwMaxSize = SIZE                                                   \
        };
#define MODUS_MSG_GET_HANDLE(NAME) &(t##NAME##Msg)   

#define MODUS_MSG_UPDATE(NAME, MESSAGE, LENGTH)                                 \
    do{                                                                         \
        if((t##NAME##Msg).hwMaxSize >= LENGTH)                                  \
        {                                                                       \
            memcpy((t##NAME##Msg).pchMessage, MESSAGE, LENGTH);                 \
            (t##NAME##Msg).hwLength = LENGTH;                                   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            (t##NAME##Msg).hwLength = 0;                                        \
        }                                                                       \
    }while(0)

#define MODUS_MSG_GET_BUFFER(NAME) (t##NAME##Msg).pchMessage
#define MODUS_MSG_GET_LENGTH(NAME) (t##NAME##Msg).hwLength

#endif // __MODUS_BASE_H__
