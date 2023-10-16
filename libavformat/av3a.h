/*
 * AVS3 Audio/Audio Vivid Codec common code
 *
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
#ifdef CONFIG_AV3A_PARSER
#ifndef AVFORMAT_AV3A_H
#define AVFORMAT_AV3A_H

#include <stdint.h>

/* AATF header */
#define AV3A_MAX_NBYTES_HEADER    9
#define AV3A_AUDIO_SYNC_WORD      0xFFF
#define AV3A_AUDIO_FRAME_SIZE     1024
#define AV3A_SIZE_BITRATE_TABLE   16
#define AV3A_SIZE_FS_TABLE        9
#define AV3A_MC_CONFIG_TABLE_SIZE 10
#define AV3A_CHANNEL_LAYOUT       12

typedef struct {
    const char *tag;
    uint8_t channels;
    uint64_t channel_layout;
} AV3AChannelLayout;

extern int av3a_get_sampling_rate(const int sr_idx);

extern AV3AChannelLayout av3a_get_channel_layout(const int ch_idx);

#endif /* AVFORMAT_AV3A_H */
#endif /* CONFIG_AV3A_PARSER */