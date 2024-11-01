/*
 * AV3A Format Common Header
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

#ifndef AVCODEC_AV3A_H
#define AVCODEC_AV3A_H

#include "libavutil/samplefmt.h"

/* AATF header */
#define AV3A_MAX_NBYTES_HEADER          9
#define AV3A_AUDIO_SYNC_WORD            0xFFF
#define AV3A_AUDIO_FRAME_SIZE           1024
#define AV3A_CHANNEL_LAYOUT_SIZE        15
#define AV3A_SIZE_BITRATE_TABLE         16
#define AV3A_SIZE_FS_TABLE              9
#define AV3A_SIZE_RESOLUTION_TABLE      3

/* Channel Layout */
#define AV3A_CH_LAYOUT_MONO             (AV_CH_LAYOUT_MONO)
#define AV3A_CH_LAYOUT_STEREO           (AV_CH_LAYOUT_STEREO)
#define AV3A_CH_LAYOUT_4POINT0          (AV3A_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER|AV_CH_BACK_CENTER)
#define AV3A_CH_LAYOUT_5POINT1          (AV_CH_LAYOUT_5POINT1)
#define AV3A_CH_LAYOUT_7POINT1          (AV_CH_LAYOUT_5POINT1|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT)
#define AV3A_CH_LAYOUT_5POINT1POINT2    (AV_CH_LAYOUT_5POINT1|AV_CH_TOP_SIDE_LEFT|AV_CH_TOP_SIDE_RIGHT)
#define AV3A_CH_LAYOUT_7POINT1POINT2    (AV3A_CH_LAYOUT_7POINT1|AV_CH_TOP_SIDE_LEFT|AV_CH_TOP_SIDE_RIGHT)
#define AV3A_CH_LAYOUT_5POINT1POINT4    (AV_CH_LAYOUT_5POINT1|AV_CH_TOP_FRONT_LEFT|AV_CH_TOP_FRONT_RIGHT|AV_CH_TOP_BACK_LEFT|AV_CH_TOP_BACK_RIGHT)
#define AV3A_CH_LAYOUT_7POINT1POINT4    (AV3A_CH_LAYOUT_7POINT1|AV_CH_TOP_FRONT_LEFT|AV_CH_TOP_FRONT_RIGHT|AV_CH_TOP_BACK_LEFT|AV_CH_TOP_BACK_RIGHT)
#define AV3A_CH_AUDIO_OBJECT            (AV_CHAN_UNKNOWN)

typedef enum {
    CHANNEL_CONFIG_MONO       = 0,  /* Mono    = 0  */
    CHANNEL_CONFIG_STEREO     = 1,  /* Stereo  = 1  */
    CHANNEL_CONFIG_MC_5_1     = 2,  /* 5.1     = 2  */
    CHANNEL_CONFIG_MC_7_1     = 3,  /* 7.1     = 3  */
    CHANNEL_CONFIG_MC_10_2    = 4,  /* 10.2    = 4  */
    CHANNEL_CONFIG_MC_22_2    = 5,  /* 22.2    = 5  */
    CHANNEL_CONFIG_MC_4_0     = 6,  /* 4.0     = 6  */
    CHANNEL_CONFIG_MC_5_1_2   = 7,  /* 5.1.2   = 7  */
    CHANNEL_CONFIG_MC_5_1_4   = 8,  /* 5.1.4   = 8  */
    CHANNEL_CONFIG_MC_7_1_2   = 9,  /* 7.1.2   = 9  */
    CHANNEL_CONFIG_MC_7_1_4   = 10, /* 7.1.4   = 10 */
    CHANNEL_CONFIG_HOA_ORDER1 = 11, /* FOA     = 11 */
    CHANNEL_CONFIG_HOA_ORDER2 = 12, /* HOA2    = 12 */
    CHANNEL_CONFIG_HOA_ORDER3 = 13, /* HOA3    = 13 */
    CHANNEL_CONFIG_UNKNOWN    = 14  /* UNKNOWN = 14 */
} AV3AChannelConfig;

typedef struct {
    int16_t sync_word;                 /* sync word */
    int16_t audio_codec_id;            /* audio codec id */
    int16_t anc_data;                  /* anc data */
    int16_t nn_type;                   /* neural network type */
    int16_t coding_profile;            /* coding profile */
    int16_t sampling_frequency_index;  /* samping rate index */
    int16_t channel_number_index;      /* channel number index */
    int16_t bitrate_index;             /* bitrate index */
    int16_t soundBedType;              /* soundbed type  */
    int16_t object_channel_number;     /* object channel number */
    int16_t bitrate_index_per_channel; /* bitrate per object */
    int16_t order;                     /* ambisonics order */
    int16_t resolution_index;          /* resolution index */

    int32_t sampling_rate;             /* sampling rate */
    int64_t total_bitrate;             /* total bitrate */
    int16_t sample_format ;            /* sample format */
    int16_t resolution;                /* resolution */
    int16_t content_type;              /* internal content type */
    int16_t nb_channels;               /* number of channels (channel configuration) */
    int16_t nb_objects;                /* number of objects (object_channel_number + 1) */
    int16_t total_channels;            /* total channels */
    int16_t hoa_order;                 /* ambisonics order (order + 1) */
    int32_t ch_layout_mask;            /* channel layout mask */
} AATFHeaderInfo;

// bitrate table for mono
static const int64_t ff_av3a_mono_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        16000, 32000, 44000, 56000, 64000, 72000, 80000, 96000, 128000, 144000,
        164000, 192000, 0, 0, 0, 0};

// bitrate table for stereo
static const int64_t ff_av3a_stereo_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        24000, 32000, 48000, 64000, 80000, 96000, 128000, 144000, 192000, 256000,
        320000, 0, 0, 0, 0, 0};

// bitrate table for MC 5.1
static const int64_t ff_av3a_mc5p1_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        192000, 256000, 320000, 384000, 448000, 512000, 640000, 720000, 144000, 96000,
        128000, 160000, 0, 0, 0, 0};

// bitrate table for MC 7.1
static const int64_t ff_av3a_mc7p1_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        192000, 480000, 256000, 384000, 576000, 640000, 128000, 160000, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for MC 4.0
static const int64_t ff_av3a_mc4p0_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        48000, 96000, 128000, 192000, 256000, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for MC 5.1.2
static const int64_t ff_av3a_mc5p1p2_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        152000, 320000, 480000, 576000, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for MC 5.1.4
static const int64_t ff_av3a_mc5p1p4_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        176000, 384000, 576000, 704000, 256000, 448000, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for MC 7.1.2
static const int64_t ff_av3a_mc7p1p2_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        216000, 480000, 576000, 384000, 768000, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for MC 7.1.4
static const int64_t ff_av3a_mc7p1p4_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        240000, 608000, 384000, 512000, 832000, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

static const int64_t ff_av3a_foa_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        48000, 96000, 128000, 192000, 256000, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

static const int64_t ff_av3a_hoa2_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        192000, 256000, 320000, 384000, 480000, 512000, 640000, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

// bitrate table for HOA order 3
static const int64_t ff_av3a_hoa3_bitrate_table[AV3A_SIZE_BITRATE_TABLE] = {
        256000, 320000, 384000, 512000, 640000, 896000, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0};

static const int32_t ff_av3a_sampling_rate_table[AV3A_SIZE_FS_TABLE] = {
        192000, 96000, 48000, 44100, 32000, 24000, 22050, 16000, 8000
};

typedef struct {
    int16_t resolution;
    enum AVSampleFormat sample_format;
} Av3aSampleFormatMap;

static const Av3aSampleFormatMap ff_av3a_sample_format_map_table[AV3A_SIZE_RESOLUTION_TABLE] = {
        {8,  AV_SAMPLE_FMT_U8 }, /* 0: 8 bits  */
        {16, AV_SAMPLE_FMT_S16}, /* 1: 16 bits */
        {24, AV_SAMPLE_FMT_S32}, /* 2: 24 bits */
};

typedef struct {
    AV3AChannelConfig channel_number_index;
    int16_t channels;
    const enum AVChannel* channel_layout;
    uint64_t mask;
} Av3aChannelConfigMap;

static const enum AVChannel ff_av3a_default_channel_layout_mono[1] ={
        AV_CHAN_FRONT_CENTER
};

static const enum AVChannel ff_av3a_default_channel_layout_stereo[2] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT
};

static const enum AVChannel ff_av3a_channel_layout_mc_4_0[4] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT,
        AV_CHAN_FRONT_CENTER, AV_CHAN_BACK_CENTER
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_5_1[6] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_5_1_2[8] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT,
        AV_CHAN_TOP_SIDE_LEFT, AV_CHAN_TOP_SIDE_RIGHT
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_7_1[8] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT,
        AV_CHAN_BACK_LEFT, AV_CHAN_BACK_RIGHT
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_5_1_4[10] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT,
        AV_CHAN_TOP_FRONT_LEFT, AV_CHAN_TOP_FRONT_RIGHT,
        AV_CHAN_TOP_BACK_LEFT, AV_CHAN_TOP_BACK_RIGHT
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_7_1_2[10] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT,
        AV_CHAN_BACK_LEFT, AV_CHAN_BACK_RIGHT,
        AV_CHAN_TOP_SIDE_LEFT, AV_CHAN_TOP_SIDE_RIGHT
};

static const enum AVChannel ff_av3a_default_channel_layout_mc_7_1_4[12] ={
        AV_CHAN_FRONT_LEFT, AV_CHAN_FRONT_RIGHT, AV_CHAN_FRONT_CENTER, 
        AV_CHAN_LOW_FREQUENCY,
        AV_CHAN_SIDE_LEFT, AV_CHAN_SIDE_RIGHT,
        AV_CHAN_BACK_LEFT, AV_CHAN_BACK_RIGHT,
        AV_CHAN_TOP_FRONT_LEFT, AV_CHAN_TOP_FRONT_RIGHT,
        AV_CHAN_TOP_BACK_LEFT, AV_CHAN_TOP_BACK_RIGHT
};

static const Av3aChannelConfigMap ff_av3a_channels_map_table[AV3A_CHANNEL_LAYOUT_SIZE] = {
    { CHANNEL_CONFIG_MONO,       1,  ff_av3a_default_channel_layout_mono  ,   AV3A_CH_LAYOUT_MONO,         },              
    { CHANNEL_CONFIG_STEREO,     2,  ff_av3a_default_channel_layout_stereo,   AV3A_CH_LAYOUT_STEREO        },         
    { CHANNEL_CONFIG_MC_5_1,     6,  ff_av3a_default_channel_layout_mc_5_1,   AV3A_CH_LAYOUT_5POINT1       },         
    { CHANNEL_CONFIG_MC_7_1,     8,  ff_av3a_default_channel_layout_mc_7_1,   AV3A_CH_LAYOUT_7POINT1       },          
    { CHANNEL_CONFIG_MC_10_2,    12, NULL,                                    0L                           }, /* reserved */                                                            
    { CHANNEL_CONFIG_MC_22_2,    24, NULL,                                    0L                           }, /* reserved */                                                          
    { CHANNEL_CONFIG_MC_4_0,     4,  ff_av3a_channel_layout_mc_4_0,           AV3A_CH_LAYOUT_4POINT0       },                   
    { CHANNEL_CONFIG_MC_5_1_2,   8,  ff_av3a_default_channel_layout_mc_5_1_2, AV3A_CH_LAYOUT_5POINT1POINT2 },
    { CHANNEL_CONFIG_MC_5_1_4,   10, ff_av3a_default_channel_layout_mc_5_1_4, AV3A_CH_LAYOUT_5POINT1POINT4 },
    { CHANNEL_CONFIG_MC_7_1_2,   10, ff_av3a_default_channel_layout_mc_7_1_2, AV3A_CH_LAYOUT_7POINT1POINT2 },
    { CHANNEL_CONFIG_MC_7_1_4,   12, ff_av3a_default_channel_layout_mc_7_1_4, AV3A_CH_LAYOUT_7POINT1POINT4 },
    { CHANNEL_CONFIG_HOA_ORDER1, 4,  NULL,                                    0L                           },                                              
    { CHANNEL_CONFIG_HOA_ORDER2, 9,  NULL,                                    0L                           },                                             
    { CHANNEL_CONFIG_HOA_ORDER3, 16, NULL,                                    0L                           },                                            
    { CHANNEL_CONFIG_UNKNOWN,    0,  NULL,                                    0L                           },                                                                
};

typedef struct {
    AV3AChannelConfig channel_number_index;
    const int64_t *bitrate_table;
} Av3aBitrateMap;

static const Av3aBitrateMap ff_av3a_bitrate_map_table[15] = {
    {CHANNEL_CONFIG_MONO,       ff_av3a_mono_bitrate_table    },   
    {CHANNEL_CONFIG_STEREO,     ff_av3a_stereo_bitrate_table  },  
    {CHANNEL_CONFIG_MC_5_1,     ff_av3a_mc5p1_bitrate_table   },   
    {CHANNEL_CONFIG_MC_7_1,     ff_av3a_mc7p1_bitrate_table   },   
    {CHANNEL_CONFIG_MC_10_2,    NULL                          }, /* reserved */                           
    {CHANNEL_CONFIG_MC_22_2,    NULL                          }, /* reserved */               
    {CHANNEL_CONFIG_MC_4_0,     ff_av3a_mc4p0_bitrate_table   },       
    {CHANNEL_CONFIG_MC_5_1_2,   ff_av3a_mc5p1p2_bitrate_table },     
    {CHANNEL_CONFIG_MC_5_1_4,   ff_av3a_mc5p1p4_bitrate_table },     
    {CHANNEL_CONFIG_MC_7_1_2,   ff_av3a_mc7p1p2_bitrate_table },     
    {CHANNEL_CONFIG_MC_7_1_4,   ff_av3a_mc7p1p4_bitrate_table },     
    {CHANNEL_CONFIG_HOA_ORDER1, ff_av3a_foa_bitrate_table     },         
    {CHANNEL_CONFIG_HOA_ORDER2, ff_av3a_hoa2_bitrate_table    },        
    {CHANNEL_CONFIG_HOA_ORDER3, ff_av3a_hoa3_bitrate_table    },        
    {CHANNEL_CONFIG_UNKNOWN,    NULL                          },                             
};
#endif /* AVCODEC_AV3A_H */
