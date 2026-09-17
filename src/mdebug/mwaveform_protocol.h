#ifndef __MWAVEFORM_PROTOCOL_H__
#define __MWAVEFORM_PROTOCOL_H__

#include <stdint.h>

#define MWAVEFORM_PROTOCOL_MAX_CHANNELS     16
#define MWAVEFORM_PROTOCOL_VERSION          1u

#define MWAVEFORM_FRAME_TYPE_DESC            0xFD
#define MWAVEFORM_FRAME_TYPE_META            0xFE
#define MWAVEFORM_FRAME_TYPE_BATCH           0xFC

typedef struct {
    char    achName[8];
    float   fScale;
} mwaveform_ch_desc_t;

typedef struct {
    uint32_t wSampleIndex;
    uint8_t  abMask[(MWAVEFORM_PROTOCOL_MAX_CHANNELS + 7) / 8];
    int16_t  ahwSamples[MWAVEFORM_PROTOCOL_MAX_CHANNELS];
} mwaveform_batch_sample_t;

typedef struct {
    /**
     * @brief Pack data frame
     * @param pchBuffer Output buffer
     * @param ahwSamples Array of samples
     * @param abMask Mask of active channels
     * @param chCount Total number of registered channels
     * @param chSeq Sequence number
     * @return uint16_t Length of packed frame
     */
    uint16_t (*pack_data)(uint8_t *pchBuffer, const int16_t *ahwSamples, 
                          const uint8_t *abMask, uint8_t chCount, uint8_t chSeq);
    
    /**
     * @brief Pack descriptor frame
     * @param pchBuffer Output buffer
     * @param atChannels Array of channel descriptors
     * @param chCount Total number of registered channels
     * @return uint16_t Length of packed frame
     */
    uint16_t (*pack_desc)(uint8_t *pchBuffer, 
                          const mwaveform_ch_desc_t *atChannels, uint8_t chCount);

    /**
     * @brief Pack stream metadata frame
     */
    uint16_t (*pack_meta)(uint8_t *pchBuffer, uint8_t chCount,
                          uint32_t periodNs, uint16_t batchDepth);

    /**
     * @brief Pack one batch of stream samples
     */
    uint16_t (*pack_batch)(uint8_t *pchBuffer,
                           const mwaveform_batch_sample_t *atSamples,
                           uint8_t chCount, uint16_t ringDepth,
                           uint16_t startOffset, uint16_t sampleCount,
                           uint32_t startSampleIndex, uint32_t periodNs);

} mwaveform_protocol_t;

extern const mwaveform_protocol_t default_waveform_protocol;

#endif /* __MWAVEFORM_PROTOCOL_H__ */
