/*
 * AV3A Demuxer
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
#include "avformat.h"
#include "avio_internal.h"
#include "internal.h"
#include "rawdec.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/channel_layout.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/av3a.h"
#include <string.h>

typedef struct {
    uint8_t audio_codec_id;
    uint8_t sampling_frequency_index;
    uint8_t nn_type;
    uint8_t content_type;
    uint8_t channel_number_index;
    uint8_t number_objects;
    uint8_t hoa_order;
    uint8_t resolution_index;
    uint16_t total_bitrate_kbps;
} Av3aFormatContext;

static int av3a_read_aatf_frame_header(AATFHeaderInfo *hdf, const uint8_t *buf)
{
    int16_t sync_word;
    GetBitContext gb;

    hdf->nb_channels = 0;
    hdf->nb_objects  = 0;

    init_get_bits8(&gb, buf, AV3A_MAX_NBYTES_HEADER);

    sync_word = get_bits(&gb, 12);
    if (sync_word != AV3A_AUDIO_SYNC_WORD) {
        return AVERROR_INVALIDDATA;
    }

    /* codec id */
    hdf->audio_codec_id = get_bits(&gb, 4);
    if (hdf->audio_codec_id != AV3A_LOSSY_CODEC_ID) {
        return AVERROR_INVALIDDATA;
    }

    /* anc data */
    hdf->anc_data = get_bits(&gb, 1);
    if (hdf->anc_data) {
        return AVERROR_INVALIDDATA;
    }

    /* neural network type */
    hdf->nn_type = get_bits(&gb, 3);
    if ((hdf->nn_type > AV3A_LC_NN_TYPE) || (hdf->nn_type < AV3A_BASELINE_NN_TYPE)) {
        return AVERROR_INVALIDDATA;
    }

    /* coding profile */
    hdf->coding_profile = get_bits(&gb, 3);

    /* sampling rate */
    hdf->sampling_frequency_index = get_bits(&gb, 4);
    if ((hdf->sampling_frequency_index >= AV3A_FS_TABLE_SIZE) || (hdf->sampling_frequency_index < 0)) {
        return AVERROR_INVALIDDATA;
    }
    hdf->sampling_rate = ff_av3a_sampling_rate_table[hdf->sampling_frequency_index];

    skip_bits(&gb, 8);

    if (hdf->coding_profile == AV3A_BASE_PROFILE) {
        hdf->content_type         = AV3A_CHANNEL_BASED_TYPE;
        hdf->channel_number_index = get_bits(&gb, 7);
        if ((hdf->channel_number_index >= CHANNEL_CONFIG_UNKNOWN) ||
            (hdf->channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
            (hdf->channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
            (hdf->channel_number_index < 0)) {
            return AVERROR_INVALIDDATA;
        }
        hdf->nb_channels = ff_av3a_channels_map_table[hdf->channel_number_index].channels;
    } else if (hdf->coding_profile == AV3A_OBJECT_METADATA_PROFILE) {
        hdf->soundbed_type = get_bits(&gb, 2);
        if (hdf->soundbed_type == 0) {
            hdf->content_type              = AV3A_OBJECT_BASED_TYPE;
            hdf->object_channel_number     = get_bits(&gb, 7);
            if (hdf->object_channel_number < 0) {
                return AVERROR_INVALIDDATA;
            }
            hdf->bitrate_index_per_channel = get_bits(&gb, 4);
            if ((hdf->bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_objects    = hdf->object_channel_number + 1;
            hdf->total_bitrate = ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[hdf->bitrate_index_per_channel] * hdf->nb_objects;
        } else if (hdf->soundbed_type == 1) {
            hdf->content_type = AV3A_CHANNEL_OBJECT_TYPE;
            hdf->channel_number_index = get_bits(&gb, 7);
            if ((hdf->channel_number_index >= CHANNEL_CONFIG_UNKNOWN) ||
                (hdf->channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
                (hdf->channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
                (hdf->channel_number_index < 0)) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_channels   = ff_av3a_channels_map_table[hdf->channel_number_index].channels;
            hdf->bitrate_index = get_bits(&gb, 4);
            if ((hdf->bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index < 0)) {
                return AVERROR_INVALIDDATA;
            }

            hdf->object_channel_number = get_bits(&gb, 7);
            if (hdf->object_channel_number < 0) {
                return AVERROR_INVALIDDATA;
            }
            hdf->nb_objects                = hdf->object_channel_number + 1;
            hdf->bitrate_index_per_channel = get_bits(&gb, 4);
            if ((hdf->bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }

            hdf->total_bitrate = ff_av3a_bitrate_map_table[hdf->channel_number_index].bitrate_table[hdf->bitrate_index] +
                ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[hdf->bitrate_index_per_channel] * hdf->nb_objects;
        } else {
            return AVERROR_INVALIDDATA;
        }
    } else if (hdf->coding_profile == AV3A_AMBISONIC_PROFILE) {
        hdf->content_type = AV3A_AMBISONIC_TYPE;
        hdf->order        = get_bits(&gb, 4);
        hdf->hoa_order    = hdf->order + 1;

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

    /* resolution */
    hdf->resolution_index = get_bits(&gb, 2);
    if ((hdf->resolution_index >= AV3A_RESOLUTION_TABLE_SIZE) || (hdf->resolution_index < 0)) {
        return AVERROR_INVALIDDATA;
    }
    hdf->resolution    = ff_av3a_sample_format_map_table[hdf->resolution_index].resolution;
    hdf->sample_format = ff_av3a_sample_format_map_table[hdf->resolution_index].sample_format;

    if (hdf->coding_profile != AV3A_OBJECT_METADATA_PROFILE) {
        hdf->bitrate_index  = get_bits(&gb, 4);
        if ((hdf->bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (hdf->bitrate_index < 0)) {
                return AVERROR_INVALIDDATA;
        }
        hdf->total_bitrate = ff_av3a_bitrate_map_table[hdf->channel_number_index].bitrate_table[hdf->bitrate_index];
    }

    skip_bits(&gb, 8);

    return 0;
}

static int av3a_get_packet_size(AVFormatContext *s)
{
    int ret = 0;
    int read_bytes = 0;
    uint16_t sync_word = 0;
    int payload_bytes = 0;
    int payloud_bits  = 0;
    uint8_t header[AV3A_MAX_NBYTES_HEADER];
    GetBitContext gb;
    int32_t sampling_rate;
    int16_t coding_profile, sampling_frequency_index, channel_number_index;
    int16_t bitrate_index, bitrate_index_per_channel;
    int16_t objects, hoa_order;
    int64_t total_bitrate;

    if (!s) {
        return AVERROR(ENOMEM);
    }

    if (!s->pb) {
        return AVERROR(ENOMEM);
    }

    read_bytes = avio_read(s->pb, header, AV3A_MAX_NBYTES_HEADER);
    if (read_bytes != AV3A_MAX_NBYTES_HEADER) {
        return (read_bytes < 0) ? read_bytes : AVERROR_EOF;
    }

    init_get_bits8(&gb, header, AV3A_MAX_NBYTES_HEADER);

    sync_word = get_bits(&gb, 12);
    if (sync_word != AV3A_AUDIO_SYNC_WORD) {
        return AVERROR_INVALIDDATA;
    }

    skip_bits(&gb, 8);

    coding_profile            = get_bits(&gb, 3);
    sampling_frequency_index  = get_bits(&gb, 4);
    if ((sampling_frequency_index >= AV3A_FS_TABLE_SIZE) || (sampling_frequency_index < 0)) {
        return AVERROR_INVALIDDATA;
    }
    sampling_rate = ff_av3a_sampling_rate_table[sampling_frequency_index];

    skip_bits(&gb, 8);

    if (coding_profile == AV3A_BASE_PROFILE) {
        channel_number_index = get_bits(&gb, 7);
        if ((channel_number_index >= CHANNEL_CONFIG_UNKNOWN) ||
            (channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
            (channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
            (channel_number_index < 0)) {
                return AVERROR_INVALIDDATA;
        }
    } else if (coding_profile == AV3A_OBJECT_METADATA_PROFILE) {
        int64_t soundbed_bitrate, objects_bitrate;
        int16_t soundbed_type = get_bits(&gb, 2);
        if (soundbed_type == 0) {
            objects = get_bits(&gb, 7);
            if (objects < 0) {
                return AVERROR_INVALIDDATA;
            }
            objects += 1;

            bitrate_index_per_channel = get_bits(&gb, 4);
            if ((bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }
            total_bitrate = ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[bitrate_index_per_channel] * objects;
        } else if (soundbed_type == 1) {
            channel_number_index = get_bits(&gb, 7);
            if ((channel_number_index >= CHANNEL_CONFIG_UNKNOWN) ||
                (channel_number_index == CHANNEL_CONFIG_MC_10_2) ||
                (channel_number_index == CHANNEL_CONFIG_MC_22_2) ||
                (channel_number_index < 0)) {
                return AVERROR_INVALIDDATA;
            }
            
            bitrate_index = get_bits(&gb, 4);
            if ((bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (bitrate_index < 0)) {
                return AVERROR_INVALIDDATA;
            }
            soundbed_bitrate = ff_av3a_bitrate_map_table[channel_number_index].bitrate_table[bitrate_index];

            objects = get_bits(&gb, 7);
            if (objects < 0) {
                return AVERROR_INVALIDDATA;
            }
            objects += 1;
            bitrate_index_per_channel = get_bits(&gb, 4);
            if ((bitrate_index_per_channel >= AV3A_BITRATE_TABLE_SIZE) || (bitrate_index_per_channel < 0)) {
                return AVERROR_INVALIDDATA;
            }

            objects_bitrate = ff_av3a_bitrate_map_table[CHANNEL_CONFIG_MONO].bitrate_table[bitrate_index_per_channel];
            total_bitrate   = soundbed_bitrate + (objects_bitrate * objects);
        } else {
            return AVERROR_INVALIDDATA;
        }
    } else if (coding_profile == AV3A_AMBISONIC_PROFILE) {
        hoa_order = get_bits(&gb, 4);
        hoa_order += 1;

        switch (hoa_order) {
        case AV3A_AMBISONIC_FIRST_ORDER:
            channel_number_index = CHANNEL_CONFIG_HOA_ORDER1;
            break;
        case AV3A_AMBISONIC_SECOND_ORDER:
            channel_number_index = CHANNEL_CONFIG_HOA_ORDER2;
            break;
        case AV3A_AMBISONIC_THIRD_ORDER:
            channel_number_index = CHANNEL_CONFIG_HOA_ORDER3;
            break;
        default:
            return AVERROR_INVALIDDATA;
        }
    } else {
        return AVERROR_INVALIDDATA;
    }

    skip_bits(&gb, 2);
    if (coding_profile != AV3A_OBJECT_METADATA_PROFILE) {
        bitrate_index = get_bits(&gb, 4);
        if ((bitrate_index >= AV3A_BITRATE_TABLE_SIZE) || (bitrate_index < 0)) {
            return AVERROR_INVALIDDATA;
        }
        total_bitrate = ff_av3a_bitrate_map_table[channel_number_index].bitrate_table[bitrate_index];
    }

    skip_bits(&gb, 8);

    if (sampling_rate == 44100) {
        payloud_bits  = (int)floor(((float) (total_bitrate) / sampling_rate) * AV3A_AUDIO_FRAME_SIZE);
        payload_bytes = (int)ceil((float)payloud_bits / 8);
    } else {
        payload_bytes = (int)ceil((((float) (total_bitrate) / sampling_rate) * AV3A_AUDIO_FRAME_SIZE) / 8);
    }

    if ((ret = avio_seek(s->pb, -read_bytes, SEEK_CUR)) < 0) {
        return ret;
    }

    return payload_bytes;
}

static int av3a_probe(const AVProbeData *p)
{
    uint16_t frame_sync_word;
    uint16_t lval = ((uint16_t)(p->buf[0]));
    uint16_t rval = ((uint16_t)(p->buf[1]));
    frame_sync_word = ((lval << 8) | rval) >> 4;

    if (frame_sync_word == AV3A_AUDIO_SYNC_WORD && av_match_ext(p->filename, "av3a")) {
        return AVPROBE_SCORE_MAX;
    }

    return 0;
}

static int av3a_read_header(AVFormatContext *s)
{
    int ret = 0;
    uint8_t header[AV3A_MAX_NBYTES_HEADER];
    AVStream *stream = NULL;
    Av3aFormatContext av3afmtctx;
    AATFHeaderInfo hdf;

    if (!s) {
        return AVERROR(ENOMEM);
    }

    if (!(stream = avformat_new_stream(s, NULL))) {
        return AVERROR(ENOMEM);
    }

    stream->start_time             = 0;
    ffstream(stream)->need_parsing = AVSTREAM_PARSE_FULL_RAW;
    stream->codecpar->codec_type   = AVMEDIA_TYPE_AUDIO;
    stream->codecpar->codec_id     = s->iformat->raw_codec_id;
    stream->codecpar->codec_tag    = MKTAG('a', 'v', '3', 'a');

    if ((ret = avio_read(s->pb, header, AV3A_MAX_NBYTES_HEADER)) != AV3A_MAX_NBYTES_HEADER) {
        return (ret < 0) ? ret : AVERROR_EOF;
    }

    ret = av3a_read_aatf_frame_header(&hdf, header);
    if (ret) {
        return ret;
    }

    /* stream parameters */
    stream->codecpar->format                = hdf.sample_format;
    stream->codecpar->bits_per_raw_sample   = hdf.resolution;
    stream->codecpar->bit_rate              = hdf.total_bitrate;
    stream->codecpar->sample_rate           = (int) (hdf.sampling_rate);
    stream->codecpar->frame_size            = AV3A_AUDIO_FRAME_SIZE;
    stream->codecpar->ch_layout.order       = AV_CHANNEL_ORDER_UNSPEC;
    stream->codecpar->ch_layout.nb_channels = hdf.total_channels;

    /* extradata */
    av3afmtctx.audio_codec_id           = hdf.audio_codec_id;
    av3afmtctx.sampling_frequency_index = hdf.sampling_frequency_index;
    av3afmtctx.nn_type                  = hdf.nn_type;
    av3afmtctx.content_type             = hdf.content_type;
    av3afmtctx.channel_number_index     = hdf.channel_number_index;
    av3afmtctx.number_objects           = hdf.nb_objects;
    av3afmtctx.hoa_order                = hdf.hoa_order;
    av3afmtctx.resolution_index         = hdf.resolution_index;
    av3afmtctx.total_bitrate_kbps       = (int) (hdf.total_bitrate / 1000);

    if ((ret = ff_alloc_extradata(stream->codecpar, sizeof(Av3aFormatContext))) < 0) {
        return ret;
    }
    memcpy(stream->codecpar->extradata, &av3afmtctx, sizeof(Av3aFormatContext));

    if ((ret = avio_seek(s->pb, -AV3A_MAX_NBYTES_HEADER, SEEK_CUR)) < 0) {
        return ret;
    }

    return 0;
}

static int av3a_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int64_t pos;
    int packet_size = 0;
    int read_bytes = 0;
    int ret = 0;

    if (!s) {
        return AVERROR(ENOMEM);
    }

    if (avio_feof(s->pb)) {
        return AVERROR_EOF;
    }
    pos = avio_tell(s->pb);

    if (!(packet_size = av3a_get_packet_size(s))) {
        return AVERROR_EOF;
    }

    if (packet_size < 0) {
        return packet_size;
    }
 
    if ((ret = av_new_packet(pkt, packet_size)) < 0) {
        return ret;
    }

    if (!s->streams[0]) {
        return AVERROR(ENOMEM);
    }

    if (!s->streams[0]->codecpar) {
        return AVERROR(ENOMEM);
    }

    pkt->stream_index = 0;
    pkt->pos          = pos;
    pkt->duration     = s->streams[0]->codecpar->frame_size;

    read_bytes = avio_read(s->pb, pkt->data, packet_size);
    if (read_bytes != packet_size) {
        return (read_bytes < 0) ? read_bytes : AVERROR_EOF;
    }

    return 0;
}

const AVInputFormat ff_av3a_demuxer = {
    .name           = "av3a",
    .long_name      = NULL_IF_CONFIG_SMALL("Audio Vivid"),
    .raw_codec_id   = AV_CODEC_ID_AVS3DA,
    .priv_data_size = sizeof(FFRawDemuxerContext),
    .read_probe     = av3a_probe,
    .read_header    = av3a_read_header,
    .read_packet    = av3a_read_packet,
    .flags          = AVFMT_GENERIC_INDEX,
    .extensions     = "av3a",
    .mime_type      = "audio/av3a",
};
