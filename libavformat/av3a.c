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
#include <stdio.h>
#include "av3a.h"

const int av3a_samplingrate_table[AV3A_SIZE_FS_TABLE] = {
        192000, 96000, 48000, 44100, 32000, 24000, 22050, 16000, 8000};

const AV3AChannelLayout av3a_channel_layout_table[AV3A_CHANNEL_LAYOUT] = {
        {"Mono",    1,  0},
        {"Stereo",  2,  0},
        {"5.1",     6,  0},
        {"7.1",     8,  0},
        {"10.2",    12, 0},
        {"22.2",    24, 0},
        {"4.0",     4,  0},
        {"5.1.2",   8,  0},
        {"5.1.4",   10, 0},
        {"7.1.2",   10, 0},
        {"7.1.4",   12, 0},
        {"Unknown", 1,  0},
};

int av3a_get_sampling_rate(const int sr_idx){
    if(sr_idx < 0 || sr_idx >= AV3A_SIZE_FS_TABLE)
        return av3a_samplingrate_table[0];
    
    return av3a_samplingrate_table[sr_idx];
}

AV3AChannelLayout av3a_get_channel_layout(const int ch_idx){
    if(ch_idx < 0 || ch_idx >= AV3A_CHANNEL_LAYOUT)
        return av3a_channel_layout_table[AV3A_CHANNEL_LAYOUT - 1];
    
    return av3a_channel_layout_table[ch_idx];
}
#endif /* CONFIG_AV3A_PARSER */