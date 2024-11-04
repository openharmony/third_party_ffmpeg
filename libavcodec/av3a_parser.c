/*
 * AV3A Parser
 *
 * Copyright (c) 2024 Shuai Liu <cqliushuai@outlook.com>
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

#include <stdio.h>
#include <stdint.h>
#include "libavutil/intreadwrite.h"
#include "parser.h"
#include "get_bits.h"
#include "av3a.h"

typedef struct {
    int16_t audio_codec_id;
    int16_t nn_type;
    int16_t frame_size;
    int16_t resolution;
    int32_t sample_rate;
    int64_t bit_rate;

    int16_t content_type;
    int16_t channel_number_index;
    int16_t nb_channels;
    int16_t nb_objects;
    int16_t total_channels;
} Av3aParseContext;


static int ff_read_av3a_header_parse(GetBitContext *gb, AATFHeaderInfo *hdf) 
{    
    int64_t soundbed_bitrate = 0L;
    int64_t object_bitrate   = 0L;

    hdf->nb_channels = 0;
    hdf->nb_objects  = 0;

    hdf->sync_word = get_bits(gb, 12);
    if (hdf->sync_word != AV3A_AUDIO_SYNC_WORD) {
        return AVERROR_INVALIDDATA;
    }

    hdf->audio_codec_id = get_bits(gb, 4);
    if (hdf->audio_codec_id != AV3A_LOSSY_CODEC_ID) {
        return AVERROR_INVALIDDATA;
    }

    skip_bits(gb, 1); /* skip anc_data 1 bit */

    hdf->nn_type        = get_bits(gb, 3);
    hdf->coding_profile = get_bits(gb, 3);

    hdf->sampling_frequency_index = get_bits(gb, 4);
    if ((hdf->sampling_frequency_index >= AV3A_FS_TABLE_SIZE) || (hdf->sampling_frequency_index < 0)) {
        return AVERROR_INVALIDDATA;
    }
    hdf->sampling_rate = ff_av3a_sampling_rate_table[hdf->sampling_frequency_index];

    skip_bits(gb, 8); /* skip CRC 8 bits */

    if (hdf->coding_profile == 0) {
        hdf->content_type         = AV3A_CHANNEL_BASED_TYPE;
        hdf->channel_number_index = get_bits(gb, 7);
        if ((hdf->channel_number_index >= CHANNEL_CONFIG_UNKNOWN) || 
            (hdf->channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
            (hdf->channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
            (hdf->channel_number_index < 0)) {
                return AVERROR_INVALIDDATA;
        }
        hdf->nb_channels = ff_av3a_channels_map_table[hdf->channel_number_index].channels;
    } else if (hdf->coding_profile == 1) {
        hdf->soundbed_type = get_bits(gb, 2);
        if (hdf->soundbed_type == 0) {
            hdf->content_type          = AV3A_OBJECT_BASED_TYPE;
            hdf->object_channel_number = get_bits(gb, 7);
            if (hdf->object_channel_number < 0) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_objects = hdf->object_channel_number + 1;

            hdf->bitrate_index_per_channel = get_bits(gb, 4);
            if ((hdf->bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }
            object_bitrate     = ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[hdf->bitrate_index_per_channel];
            hdf->total_bitrate = object_bitrate * hdf->nb_objects;
        } else if (hdf->soundbed_type == 1) {
            hdf->content_type         = AV3A_CHANNEL_OBJECT_TYPE;
            hdf->channel_number_index = get_bits(gb, 7);
            if ((hdf->channel_number_index >= CHANNEL_CONFIG_UNKNOWN) || 
                (hdf->channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
                (hdf->channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
                (hdf->channel_number_index < 0)) {
                    return AVERROR_INVALIDDATA;
            }

            hdf->bitrate_index = get_bits(gb, 4);
            if ((hdf->bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index < 0)) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_channels = ff_av3a_channels_map_table[hdf->channel_number_index].channels;
            soundbed_bitrate = ff_av3a_bitrate_map_table[hdf->channel_number_index].bitrate_table[hdf->bitrate_index];

            hdf->object_channel_number     = get_bits(gb, 7);
            if (hdf->object_channel_number < 0) {
                return AVERROR_INVALIDDATA;
            }
            hdf->bitrate_index_per_channel = get_bits(gb, 4);
            if ((hdf->bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_objects    = hdf->object_channel_number + 1;
            object_bitrate     = ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[hdf->bitrate_index_per_channel];
            hdf->total_bitrate = soundbed_bitrate + (object_bitrate * hdf->nb_objects);
        } else {
            return AVERROR_INVALIDDATA;
        }
    } else if (hdf->coding_profile == 2) {
        hdf->content_type = AV3A_AMBISONIC_TYPE;
        hdf->order        = get_bits(gb, 4);
        hdf->hoa_order    += 1;

        switch (hdf->hoa_order) {
        case AV3A_AMBISONIC_FIRST_ORDER:
            hdf->channel_number_index = CHANNEL_CONFIG_HOA_ORDER1;
            break;
        case AV3A_AMBISONIC_SECOND_ORDER:
            hdf->channel_number_index = CHANNEL_CONFIG_HOA_ORDER2;
            break;
        case AV3A_AMBISONIC_THIRD_ORDER:
            hdf->channel_number_index = CHANNEL_CONFIG_HOA_ORDER3;
            break;
        default:
            return AVERROR_INVALIDDATA;
        }
        hdf->nb_channels = ff_av3a_channels_map_table[hdf->channel_number_index].channels;
    } else {
        return AVERROR_INVALIDDATA;
    }
    hdf->total_channels = hdf->nb_channels + hdf->nb_objects;

    hdf->resolution_index = get_bits(gb, 2);
    if((hdf->resolution_index >= AV3A_RESOLUTION_TABLE_SIZE) || (hdf->resolution_index < 0)) {
        return AVERROR_INVALIDDATA;
    }
    hdf->resolution    = ff_av3a_sample_format_map_table[hdf->resolution_index].resolution;
    hdf->sample_format = ff_av3a_sample_format_map_table[hdf->resolution_index].sample_format;
    
    if (hdf->coding_profile != 1) {
        hdf->bitrate_index = get_bits(gb, 4);
        if ((hdf->bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index < 0)) {
            return AVERROR_INVALIDDATA;
        }
        hdf->total_bitrate = ff_av3a_bitrate_map_table[hdf->channel_number_index].bitrate_table[hdf->bitrate_index];
    }

    skip_bits(gb, 8); /* skip CRC 8 bits */

    return 0;
}

static int raw_av3a_parse(AVCodecParserContext *s, AVCodecContext *avctx, const uint8_t **poutbuf,
                              int32_t *poutbuf_size, const uint8_t *buf, int32_t buf_size)
{
    int ret = 0;
    uint8_t header[AV3A_MAX_NBYTES_HEADER];
    AATFHeaderInfo hdf;
    GetBitContext gb;

    if (buf_size < AV3A_MAX_NBYTES_HEADER) {
        return buf_size;
    }
    memcpy(header, buf, AV3A_MAX_NBYTES_HEADER);

    init_get_bits8(&gb, buf, AV3A_MAX_NBYTES_HEADER);
    if ((ret = ff_read_av3a_header_parse(&gb, &hdf)) != 0) {
        return ret;
    }

    avctx->codec_id            = AV_CODEC_ID_AVS3DA;
    avctx->frame_size          = AV3A_AUDIO_FRAME_SIZE;
    avctx->bits_per_raw_sample = hdf.resolution;
    avctx->sample_rate         = hdf.sampling_rate;
    avctx->bit_rate            = hdf.total_bitrate;

    avctx->ch_layout.order       = AV_CHANNEL_ORDER_UNSPEC;
    avctx->ch_layout.nb_channels = hdf.total_channels;

    *poutbuf = buf;
    *poutbuf_size = buf_size;

    return buf_size;
}

const AVCodecParser ff_av3a_parser = {
    .codec_ids = { AV_CODEC_ID_AVS3DA },
    .priv_data_size = sizeof(Av3aParseContext),
    .parser_parse = raw_av3a_parse,
};