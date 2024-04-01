/**
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVUTIL_ENCRYPTION_INFO_H
#define AVUTIL_ENCRYPTION_INFO_H

#include <stddef.h>
#include <stdint.h>

#ifdef OHOS_DRM
#define AV_DRM_KEY_ID_SIZE                    16
#define AV_DRM_IV_SIZE                        16
#define AV_DRM_MAX_SUB_SAMPLE_NUM             64
#define AV_DRM_MAX_DRM_UUID_LEN               16
#define AV_DRM_MAX_DRM_PSSH_LEN               2048
#define AV_DRM_UUID_OFFSET                    (12)

typedef enum {
    AV_DRM_ALG_CENC_UNENCRYPTED = 0x0,
    AV_DRM_ALG_CENC_AES_CTR = 0x1,
    AV_DRM_ALG_CENC_AES_WV = 0x2,
    AV_DRM_ALG_CENC_AES_CBC = 0x3,
    AV_DRM_ALG_CENC_SM4_CBC = 0x4,
    AV_DRM_ALG_CENC_SM4_CTR,
} AV_DrmCencAlgorithm;

typedef enum {
    /* key/iv/subsample set. */
    AV_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET = 0x0,
    /* key/iv/subsample not set. */
    AV_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_NOT_SET = 0x1,
} AV_DrmCencInfoMode;

struct _AV_DrmSubSample {
    uint32_t clear_header_len;
    uint32_t pay_load_len;
};
typedef struct _AV_DrmSubSample AV_DrmSubSample;

struct _AV_DrmCencInfo {
    AV_DrmCencAlgorithm algo;
    uint8_t key_id[AV_DRM_KEY_ID_SIZE];
    uint32_t key_id_len;
    uint8_t iv[AV_DRM_IV_SIZE];
    uint32_t iv_len;
    uint32_t encrypt_blocks;
    uint32_t skip_blocks;
    uint32_t first_encrypt_offset;
    AV_DrmSubSample sub_samples[AV_DRM_MAX_SUB_SAMPLE_NUM];
    uint32_t sub_sample_num;
    AV_DrmCencInfoMode mode;
};
typedef struct _AV_DrmCencInfo AV_DrmCencInfo;

struct _AV_DrmInfo {
    uint32_t uuid_len;
    uint8_t uuid[AV_DRM_MAX_DRM_UUID_LEN];
    uint32_t pssh_len;
    uint8_t pssh[AV_DRM_MAX_DRM_PSSH_LEN];
};
typedef struct _AV_DrmInfo AV_DrmInfo;
#endif

typedef struct AVSubsampleEncryptionInfo {
    /** The number of bytes that are clear. */
    unsigned int bytes_of_clear_data;

    /**
     * The number of bytes that are protected.  If using pattern encryption,
     * the pattern applies to only the protected bytes; if not using pattern
     * encryption, all these bytes are encrypted.
     */
    unsigned int bytes_of_protected_data;
} AVSubsampleEncryptionInfo;

/**
 * This describes encryption info for a packet.  This contains frame-specific
 * info for how to decrypt the packet before passing it to the decoder.
 *
 * The size of this struct is not part of the public ABI.
 */
typedef struct AVEncryptionInfo {
    /** The fourcc encryption scheme, in big-endian byte order. */
    uint32_t scheme;

    /**
     * Only used for pattern encryption.  This is the number of 16-byte blocks
     * that are encrypted.
     */
    uint32_t crypt_byte_block;

    /**
     * Only used for pattern encryption.  This is the number of 16-byte blocks
     * that are clear.
     */
    uint32_t skip_byte_block;

    /**
     * The ID of the key used to encrypt the packet.  This should always be
     * 16 bytes long, but may be changed in the future.
     */
    uint8_t *key_id;
    uint32_t key_id_size;

    /**
     * The initialization vector.  This may have been zero-filled to be the
     * correct block size.  This should always be 16 bytes long, but may be
     * changed in the future.
     */
    uint8_t *iv;
    uint32_t iv_size;

    /**
     * An array of subsample encryption info specifying how parts of the sample
     * are encrypted.  If there are no subsamples, then the whole sample is
     * encrypted.
     */
    AVSubsampleEncryptionInfo *subsamples;
    uint32_t subsample_count;
} AVEncryptionInfo;

/**
 * This describes info used to initialize an encryption key system.
 *
 * The size of this struct is not part of the public ABI.
 */
typedef struct AVEncryptionInitInfo {
    /**
     * A unique identifier for the key system this is for, can be NULL if it
     * is not known.  This should always be 16 bytes, but may change in the
     * future.
     */
    uint8_t* system_id;
    uint32_t system_id_size;

    /**
     * An array of key IDs this initialization data is for.  All IDs are the
     * same length.  Can be NULL if there are no known key IDs.
     */
    uint8_t** key_ids;
    /** The number of key IDs. */
    uint32_t num_key_ids;
    /**
     * The number of bytes in each key ID.  This should always be 16, but may
     * change in the future.
     */
    uint32_t key_id_size;

    /**
     * Key-system specific initialization data.  This data is copied directly
     * from the file and the format depends on the specific key system.  This
     * can be NULL if there is no initialization data; in that case, there
     * will be at least one key ID.
     */
    uint8_t* data;
    uint32_t data_size;

    /**
     * An optional pointer to the next initialization info in the list.
     */
    struct AVEncryptionInitInfo *next;
} AVEncryptionInitInfo;

/**
 * Allocates an AVEncryptionInfo structure and sub-pointers to hold the given
 * number of subsamples.  This will allocate pointers for the key ID, IV,
 * and subsample entries, set the size members, and zero-initialize the rest.
 *
 * @param subsample_count The number of subsamples.
 * @param key_id_size The number of bytes in the key ID, should be 16.
 * @param iv_size The number of bytes in the IV, should be 16.
 *
 * @return The new AVEncryptionInfo structure, or NULL on error.
 */
AVEncryptionInfo *av_encryption_info_alloc(uint32_t subsample_count, uint32_t key_id_size, uint32_t iv_size);

/**
 * Allocates an AVEncryptionInfo structure with a copy of the given data.
 * @return The new AVEncryptionInfo structure, or NULL on error.
 */
AVEncryptionInfo *av_encryption_info_clone(const AVEncryptionInfo *info);

/**
 * Frees the given encryption info object.  This MUST NOT be used to free the
 * side-data data pointer, that should use normal side-data methods.
 */
void av_encryption_info_free(AVEncryptionInfo *info);

/**
 * Creates a copy of the AVEncryptionInfo that is contained in the given side
 * data.  The resulting object should be passed to av_encryption_info_free()
 * when done.
 *
 * @return The new AVEncryptionInfo structure, or NULL on error.
 */
AVEncryptionInfo *av_encryption_info_get_side_data(const uint8_t *side_data, size_t side_data_size);

/**
 * Allocates and initializes side data that holds a copy of the given encryption
 * info.  The resulting pointer should be either freed using av_free or given
 * to av_packet_add_side_data().
 *
 * @return The new side-data pointer, or NULL.
 */
uint8_t *av_encryption_info_add_side_data(
      const AVEncryptionInfo *info, size_t *side_data_size);

#ifdef OHOS_DRM
AV_DrmCencInfo *av_encryption_info_add_side_data_ex(
    const AVEncryptionInfo *info, size_t *side_data_size, AV_DrmCencInfo *cenc_info);
#endif

/**
 * Allocates an AVEncryptionInitInfo structure and sub-pointers to hold the
 * given sizes.  This will allocate pointers and set all the fields.
 *
 * @return The new AVEncryptionInitInfo structure, or NULL on error.
 */
AVEncryptionInitInfo *av_encryption_init_info_alloc(
    uint32_t system_id_size, uint32_t num_key_ids, uint32_t key_id_size, uint32_t data_size);

/**
 * Frees the given encryption init info object.  This MUST NOT be used to free
 * the side-data data pointer, that should use normal side-data methods.
 */
void av_encryption_init_info_free(AVEncryptionInitInfo* info);

/**
 * Creates a copy of the AVEncryptionInitInfo that is contained in the given
 * side data.  The resulting object should be passed to
 * av_encryption_init_info_free() when done.
 *
 * @return The new AVEncryptionInitInfo structure, or NULL on error.
 */
AVEncryptionInitInfo *av_encryption_init_info_get_side_data(
    const uint8_t* side_data, size_t side_data_size);

/**
 * Allocates and initializes side data that holds a copy of the given encryption
 * init info.  The resulting pointer should be either freed using av_free or
 * given to av_packet_add_side_data().
 *
 * @return The new side-data pointer, or NULL.
 */
uint8_t *av_encryption_init_info_add_side_data(
    const AVEncryptionInitInfo *info, size_t *side_data_size);

#endif /* AVUTIL_ENCRYPTION_INFO_H */
