/*
 * av3a parser
 *
 * Copyright (c) 2018 James Almer <jamrial@gmail.com>
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
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavutil/intreadwrite.h"
#include "parser.h"
#include "get_bits.h"

/* AVS3 header */
#define AVS3_AUDIO_HEADER_SIZE 7
#define AVS3_SYNC_WORD_SIZE 2
#define MAX_NBYTES_FRAME_HEADER 9
#define AVS3_AUDIO_SYNC_WORD 0xFFF

#define AVS3_AUDIO_FRAME_SIZE 1024
#define AVS3_SIZE_BITRATE_TABLE 16
#define AVS3_SIZE_FS_TABLE 9

/* AVS3 Audio Format */
#define AVS3_MONO_FORMAT 0
#define AVS3_STEREO_FORMAT 1
#define AVS3_MC_FORMAT 2
#define AVS3_HOA_FORMAT 3
#define AVS3_MIX_FORMAT 4

#define AVS3_SIZE_MC_CONFIG_TABLE 10

#define AVS3P3_CH_LAYOUT_5POINT1 (AV_CH_LAYOUT_SURROUND | AV_CH_LOW_FREQUENCY | AV_CH_BACK_LEFT | AV_CH_BACK_RIGHT)

typedef struct AVS3AudioParseContext {
    int32_t frame_size;
    int32_t bitdepth;
    int32_t sample_rate;
    uint64_t bit_rate;
    uint16_t channels;
    uint64_t channel_layout;
} AVS3AParseContext;

// AVS3P3 header information
typedef struct {
    // header info
    uint8_t codec_id;
    uint8_t sampling_rate_index;
    int32_t sampling_rate;

    uint16_t bitdepth;
    uint16_t channels;
    uint16_t objects;
    uint16_t hoa_order;
    uint64_t channel_layout;
    int64_t total_bitrate;

    // configuration
    uint8_t content_type;
    uint16_t channel_num_index;
    uint16_t total_channels;
    uint8_t resolution;
    uint8_t nn_type;
    uint8_t resolution_index;
} AVS3AHeaderInfo;

typedef enum {
    CHANNEL_CONFIG_MONO = 0,
    CHANNEL_CONFIG_STEREO = 1,
    CHANNEL_CONFIG_MC_5_1,
    CHANNEL_CONFIG_MC_7_1,
    CHANNEL_CONFIG_MC_10_2,
    CHANNEL_CONFIG_MC_22_2,
    CHANNEL_CONFIG_MC_4_0,
    CHANNEL_CONFIG_MC_5_1_2,
    CHANNEL_CONFIG_MC_5_1_4,
    CHANNEL_CONFIG_MC_7_1_2,
    CHANNEL_CONFIG_MC_7_1_4,
    CHANNEL_CONFIG_HOA_ORDER1,
    CHANNEL_CONFIG_HOA_ORDER2,
    CHANNEL_CONFIG_HOA_ORDER3,
    CHANNEL_CONFIG_UNKNOWN
} AVS3AChannelConfig;

/* Codec bitrate config struct */
typedef struct CodecBitrateConfigStructure {
    AVS3AChannelConfig channelNumConfig;
    const int64_t *bitrateTable;
} CodecBitrateConfig;

typedef struct McChannelConfigStructure {
    const char mcCmdString[10];
    AVS3AChannelConfig channelNumConfig;
    const int16_t numChannels;
} McChanelConfig;

static const McChanelConfig mcChannelConfigTable[AVS3_SIZE_MC_CONFIG_TABLE] = {
        {"STEREO", CHANNEL_CONFIG_STEREO, 2},
        {"MC_5_1_0", CHANNEL_CONFIG_MC_5_1, 6},
        {"MC_7_1_0", CHANNEL_CONFIG_MC_7_1, 8},
        {"MC_10_2", CHANNEL_CONFIG_MC_10_2, 12},
        {"MC_22_2", CHANNEL_CONFIG_MC_22_2, 24},
        {"MC_4_0", CHANNEL_CONFIG_MC_4_0, 4},
        {"MC_5_1_2", CHANNEL_CONFIG_MC_5_1_2, 8},
        {"MC_5_1_4", CHANNEL_CONFIG_MC_5_1_4, 10},
        {"MC_7_1_2", CHANNEL_CONFIG_MC_7_1_2, 10},
        {"MC_7_1_4", CHANNEL_CONFIG_MC_7_1_4, 12}
};

static const int32_t avs3_samplingrate_table[AVS3_SIZE_FS_TABLE] = {
    192000, 96000, 48000, 44100, 32000, 24000, 22050, 16000, 8000
};

// bitrate table for mono
static const int64_t bitrateTableMono[AVS3_SIZE_BITRATE_TABLE] = {
    16000, 32000, 44000, 56000, 64000, 72000, 80000, 96000, 128000, 144000, 164000, 192000, 0, 0, 0, 0
};

// bitrate table for stereo
static const int64_t bitrateTableStereo[AVS3_SIZE_BITRATE_TABLE] = {
    24000, 32000, 48000, 64000, 80000, 96000, 128000, 144000, 192000, 256000, 320000, 0, 0, 0, 0, 0
};

// bitrate table for MC 5.1
static const int64_t bitrateTableMC5P1[AVS3_SIZE_BITRATE_TABLE] = {
    192000, 256000, 320000, 384000, 448000, 512000, 640000, 720000, 144000, 96000, 128000, 160000, 0, 0, 0, 0
};

// bitrate table for MC 7.1
static const int64_t bitrateTableMC7P1[AVS3_SIZE_BITRATE_TABLE] = {
    192000, 480000, 256000, 384000, 576000, 640000, 128000, 160000, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for MC 4.0
static const int64_t bitrateTableMC4P0[AVS3_SIZE_BITRATE_TABLE] = {
    48000, 96000, 128000, 192000, 256000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for MC 5.1.2
static const int64_t bitrateTableMC5P1P2[AVS3_SIZE_BITRATE_TABLE] = {
    152000, 320000, 480000, 576000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for MC 5.1.4
static const int64_t bitrateTableMC5P1P4[AVS3_SIZE_BITRATE_TABLE] = {
    176000, 384000, 576000, 704000, 256000, 448000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for MC 7.1.2
static const int64_t bitrateTableMC7P1P2[AVS3_SIZE_BITRATE_TABLE] = {
    216000, 480000, 576000, 384000, 768000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for MC 7.1.4
static const int64_t bitrateTableMC7P1P4[AVS3_SIZE_BITRATE_TABLE] = {
    240000, 608000, 384000, 512000, 832000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int64_t bitrateTableFoa[AVS3_SIZE_BITRATE_TABLE] = {
    48000, 96000, 128000, 192000, 256000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int64_t bitrateTableHoa2[AVS3_SIZE_BITRATE_TABLE] = {
    192000, 256000, 320000, 384000, 480000, 512000, 640000, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// bitrate table for HOA order 3
static const int64_t bitrateTableHoa3[AVS3_SIZE_BITRATE_TABLE] = {
    256000, 320000, 384000, 512000, 640000, 896000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Codec channel number & bitrate config table
// format: {channelConfigIdx, numChannels, bitrateTable}
static const CodecBitrateConfig codecBitrateConfigTable[CHANNEL_CONFIG_UNKNOWN] = {
    {CHANNEL_CONFIG_MONO, bitrateTableMono},
    {CHANNEL_CONFIG_STEREO, bitrateTableStereo},
    {CHANNEL_CONFIG_MC_5_1, bitrateTableMC5P1},
    {CHANNEL_CONFIG_MC_7_1, bitrateTableMC7P1},
    {CHANNEL_CONFIG_MC_10_2, NULL},
    {CHANNEL_CONFIG_MC_22_2, NULL},
    {CHANNEL_CONFIG_MC_4_0, bitrateTableMC4P0},
    {CHANNEL_CONFIG_MC_5_1_2, bitrateTableMC5P1P2},
    {CHANNEL_CONFIG_MC_5_1_4, bitrateTableMC5P1P4},
    {CHANNEL_CONFIG_MC_7_1_2, bitrateTableMC7P1P2},
    {CHANNEL_CONFIG_MC_7_1_4, bitrateTableMC7P1P4},
    {CHANNEL_CONFIG_HOA_ORDER1, bitrateTableFoa},
    {CHANNEL_CONFIG_HOA_ORDER2, bitrateTableHoa2},
    {CHANNEL_CONFIG_HOA_ORDER3, bitrateTableHoa3}
};

static int read_av3a_frame_header(AVS3AHeaderInfo *hdf, const uint8_t *buf, const int32_t byte_size)
{
    GetBitContext gb;
    AVS3AChannelConfig channel_config = CHANNEL_CONFIG_UNKNOWN;

    uint8_t content_type = 0;
    uint8_t hoa_order = 0;
    uint8_t bitdepth = 0;
    uint8_t resolution = 0;

    int16_t channels = 0;
    int16_t objects = 0;
    uint64_t channel_layout = 0;

    int64_t bitrate_per_obj = 0;
    int64_t bitrate_bed_mc = 0;
    int64_t total_bitrate = 0;

    uint8_t num_chan_index = 0;
    uint8_t obj_brt_idx = 0;
    uint8_t bed_brt_idx = 0;
    uint8_t brt_idx = 0;

    // Read max header length into bs buffer
    init_get_bits8(&gb, buf, MAX_NBYTES_FRAME_HEADER);

    // 12 bits for frame sync word
    if (get_bits(&gb, 12) != AVS3_AUDIO_SYNC_WORD) {
        return AVERROR_INVALIDDATA;
    }

    // 4 bits for codec id
    uint8_t codec_id = get_bits(&gb, 4);
    if (codec_id != 2) {
        return AVERROR_INVALIDDATA;
    }

    // 1 bits for anc data
    if (get_bits(&gb, 1) != 0) {
        return AVERROR_INVALIDDATA;
    }

    // 3 bits nn type
    uint8_t nn_type = get_bits(&gb, 3);

    // 3 bits for coding profile
    uint8_t coding_profile = get_bits(&gb, 3);

    // 4 bits for sampling index
    uint8_t samping_rate_index = get_bits(&gb, 4);
    if (samping_rate_index >= AVS3_SIZE_FS_TABLE) {
        return AVERROR_INVALIDDATA;
    }

    // skip 8 bits for CRC first part
    skip_bits(&gb, 8);

    if (coding_profile == 0) {
        content_type = 0;

        // 7 bits for mono/stereo/MC
        num_chan_index = get_bits(&gb, 7);
        if (num_chan_index >= CHANNEL_CONFIG_UNKNOWN) {
            return AVERROR_INVALIDDATA;
        }

        channel_config = (AVS3AChannelConfig)num_chan_index;
        switch (channel_config) {
            case CHANNEL_CONFIG_MONO:
                channels = 1;
                channel_layout = AV_CH_LAYOUT_MONO;
                break;
            case CHANNEL_CONFIG_STEREO:
                channels = 2;
                channel_layout = AV_CH_LAYOUT_STEREO;
                break;
            case CHANNEL_CONFIG_MC_4_0:
                channels = 4;
                break;
            case CHANNEL_CONFIG_MC_5_1:
                channels = 6;
                channel_layout = AVS3P3_CH_LAYOUT_5POINT1;
                break;
            case CHANNEL_CONFIG_MC_7_1:
                channels = 8;
                channel_layout = AV_CH_LAYOUT_7POINT1;
                break;
            case CHANNEL_CONFIG_MC_5_1_2:
                channels = 8;
                break;
            case CHANNEL_CONFIG_MC_5_1_4:
                channels = 10;
                break;
            case CHANNEL_CONFIG_MC_7_1_2:
                channels = 10;
                break;
            case CHANNEL_CONFIG_MC_7_1_4:
                channels = 12;
                break;
            case CHANNEL_CONFIG_MC_22_2:
                channels = 24;
                channel_layout = AV_CH_LAYOUT_22POINT2;
                break;
            default:
                break;
        }
    } else if (coding_profile == 1) {
        // sound bed type, 2bits
        uint8_t soundBedType = get_bits(&gb, 2);

        if (soundBedType == 0) {
            content_type = 1;

            // for only objects
            // object number, 7 bits
            objects = get_bits(&gb, 7);
            objects += 1;

            // bitrate index for each obj, 4 bits
            obj_brt_idx = get_bits(&gb, 4);

            total_bitrate = codecBitrateConfigTable[CHANNEL_CONFIG_MONO].bitrateTable[obj_brt_idx] * objects;
        } else if (soundBedType == 1) {
            content_type = 2;

            // for MC + objs
            // channel number index, 7 bits
            num_chan_index = get_bits(&gb, 7);
            if (num_chan_index >= CHANNEL_CONFIG_UNKNOWN) {
                return AVERROR_INVALIDDATA;
            }

            // bitrate index for sound bed, 4 bits
            bed_brt_idx = get_bits(&gb, 4);

            // object number, 7 bits
            objects = get_bits(&gb, 7);
            objects += 1;

            // bitrate index for each obj, 4 bits
            obj_brt_idx = get_bits(&gb, 4);

            // channelNumIdx for sound bed
            channel_config = (AVS3AChannelConfig)num_chan_index;

            // sound bed bitrate
            bitrate_bed_mc = codecBitrateConfigTable[channel_config].bitrateTable[bed_brt_idx];

            // numChannels for sound bed
            for (int16_t i = 0; i < AVS3_SIZE_MC_CONFIG_TABLE; i++) {
                if (channel_config == mcChannelConfigTable[i].channelNumConfig) {
                    channels = mcChannelConfigTable[i].numChannels;
                }
            }

            // bitrate per obj
            bitrate_per_obj = codecBitrateConfigTable[CHANNEL_CONFIG_MONO].bitrateTable[obj_brt_idx];

            // add num chans and num objs to get total chans
            /* channels += objects; */

            total_bitrate = bitrate_bed_mc + bitrate_per_obj * objects;
        }
    } else if (coding_profile == 2) {
        content_type = 3;

        // 4 bits for HOA order
        hoa_order = get_bits(&gb, 4);
        hoa_order += 1;

        switch (hoa_order) {
            case 1:
                channels = 4;
                channel_config = CHANNEL_CONFIG_HOA_ORDER1;
                break;
            case 2:
                channels = 9;
                channel_config = CHANNEL_CONFIG_HOA_ORDER2;
                break;
            case 3:
                channels = 16;
                channel_config = CHANNEL_CONFIG_HOA_ORDER3;
                break;
            default:
                break;
        }
    } else {
        return AVERROR_INVALIDDATA;
    }

    // 2 bits for bit depth
    uint8_t resolution_index = get_bits(&gb, 2);
    switch (resolution_index) {
        case 0:
            bitdepth = AV_SAMPLE_FMT_U8;
            resolution = 8;
            break;
        case 1:
            bitdepth = AV_SAMPLE_FMT_S16;
            resolution = 16;
            break;
        case 2:
            resolution = 24;
            break;
        default:
            return AVERROR_INVALIDDATA;
    }

    if (coding_profile != 1) {
        // 4 bits for bitrate index
        brt_idx = get_bits(&gb, 4);
        total_bitrate = codecBitrateConfigTable[channel_config].bitrateTable[brt_idx];
    }

    // 8 bits for CRC second part
    skip_bits(&gb, 8);

    /* AVS3P6 M6954-v3 */
    hdf->codec_id = codec_id;
    hdf->sampling_rate_index = samping_rate_index;
    hdf->sampling_rate = avs3_samplingrate_table[samping_rate_index];
    hdf->bitdepth = bitdepth;

    hdf->nn_type = nn_type;
    hdf->content_type = content_type;

    if (hdf->content_type == 0) {
        hdf->channel_num_index = num_chan_index;
        hdf->channels = channels;
        hdf->objects = 0;
        hdf->total_channels = channels;
        hdf->channel_layout = channel_layout;
    } else if (hdf->content_type == 1) {
        hdf->objects = objects;
        hdf->channels = 0;
        hdf->total_channels = objects;
    } else if (hdf->content_type == 2) {
        hdf->channel_num_index = num_chan_index;
        hdf->channels = channels;
        hdf->objects = objects;
        hdf->total_channels = channels + objects;
        hdf->channel_layout = channel_layout;
    } else if (hdf->content_type == 3) {
        hdf->hoa_order = hoa_order;
        hdf->channels = channels;
        hdf->total_channels = channels;
    } else {
        return AVERROR_INVALIDDATA;
    }

    hdf->total_bitrate = total_bitrate;
    hdf->resolution = resolution;
    hdf->resolution_index = resolution_index;

    return 0;
}

static int32_t raw_av3a_parse(AVCodecParserContext *s, AVCodecContext *avctx, const uint8_t **poutbuf,
                              int32_t *poutbuf_size, const uint8_t *buf, int32_t buf_size)
{
    uint8_t header[MAX_NBYTES_FRAME_HEADER];
    AVS3AHeaderInfo hdf;
    hdf.channel_layout = 0;

    if (buf_size < MAX_NBYTES_FRAME_HEADER) {
        return buf_size;
    }

    memcpy(header, buf, MAX_NBYTES_FRAME_HEADER);

    // read frame header
    int32_t ret = read_av3a_frame_header(&hdf, header, MAX_NBYTES_FRAME_HEADER);
    if (ret != 0) {
        return ret;
    }

    avctx->sample_rate = hdf.sampling_rate;
    avctx->bit_rate = hdf.total_bitrate;
    avctx->channels = hdf.total_channels;
    avctx->channel_layout = hdf.channel_layout;
    avctx->frame_size = AVS3_AUDIO_FRAME_SIZE;
    s->format = hdf.bitdepth;

    /* return the full packet */
    *poutbuf = buf;
    *poutbuf_size = buf_size;

    return buf_size;
}

#ifdef CONFIG_AV3A_PARSER
const AVCodecParser ff_av3a_parser = {
    .codec_ids = { AV_CODEC_ID_AVS3DA },
    .priv_data_size = sizeof(AVS3AParseContext),
    .parser_parse = raw_av3a_parse,
};
#endif