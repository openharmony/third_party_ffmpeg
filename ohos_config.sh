# FFmpeg build configure

#!/bin/bash

FFMPEG_PATH=$1
FFMPEG_OUT_PATH=$2

FF_CONFIG_OPTIONS="
    --disable-lzma
    --disable-stripping
    --disable-asm
    --disable-programs
    --disable-doc
    --disable-debug
    --disable-avdevice
    --disable-avfilter
    --disable-avresample
    --disable-postproc
    --disable-bsfs
    --disable-iconv
    --disable-xlib
    --disable-zlib
    --disable-cuvid
    --disable-cuda
    --disable-libxcb
    --disable-libxcb_shape
    --disable-libxcb_shm
    --disable-libxcb_xfixes
    --disable-sdl2
    --disable-hwaccels
    --disable-protocols
    --disable-bzlib
    --disable-vaapi
    --disable-vdpau
    --enable-protocol=file,http,tcp,httpproxy,g726_16_dynamic,g726_24_dynamic,g726_32_dynamic,g726_40_dynamic,qt_rtp_aud,qt_rtp_vid,quicktime_rtp_aud,quicktime_rtp_vid
    --disable-muxers
    --disable-demuxers
    --enable-demuxer=matroska,mov,avi,flv,mpegts,mpegtsraw,mpegps,ivf
    --enable-demuxer=m4v,h263,ingenient,mjpeg,mpegvideo,rawvideo
    --enable-demuxer=mp3,wav,aac,ape,flac,amr,ogg,dsf,iff
    --enable-demuxer=pcm_alaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_mulaw,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_s8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_u8,sln,adx,g722,gsm,loas
    --disable-parsers
    --enable-parser=h263,h264,mjpeg,mpeg4video,mpegvideo,vp3,vp8,vp9
    --enable-parser=adx,aac,aac_latm,flac,g729,gsm,mpegaudio,opus,vorbis
    --disable-encoders
    --disable-decoders
    --enable-encoder=a64multi,a64multi5,amv
    --enable-decoder=h263,h263p,h264,libopenh264,h264_mediacodec,h264_mmal,h264_vda,h264_vdpau,mjpeg,rawvideo
    --enable-decoder=mpeg1_vdpau,mpeg1video,mpeg2_mmal,mpeg2video,mpeg4,mpeg4_mmal,mpeg4_vdpau,mpeg4_mediacodec,mpeg_vdpau,mpeg_xvmc,mpegvideo
    --enable-decoder=vp8,vp9,libvpx_vp8,libvpx_vp9,vp8_mediacodec,vp9_mediacodec
    --enable-decoder=pcm_alaw,pcm_alaw_at,pcm_bluray,pcm_dvd,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_lxf,pcm_mulaw,pcm_mulaw_at,pcm_s16be,pcm_s16be_planar,pcm_s16le,pcm_s16le_planar,adpcm_ima_dat4,adpcm_mtaf
    --enable-encoder=pcm_s24be,pcm_s24daud,pcm_s24le,pcm_s24le_planar,pcm_s32be,pcm_s32le,pcm_s32le_planar,pcm_s8,pcm_s8_planar,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_u8,pcm_s16le,pcm_s16le_planar,pcm_s64be,pcm_s64le,pcm_vidc,adpcm_g726,adpcm_g7261e,ssa,ass,srt,subrip,text
    --enable-decoder=mp1,mp1_at,mp1float,mp2,mp2float,mp2_at,mp3,mp3adu,mp3adufloat,mp3float,mp3on4,mp3on4float,mp3_at
    --enable-decoder=aac,aac_at,aac_fixed,aac_latm,alac,alac_at,als,amrnb,libopencore_amrnb,amr_nb_at,amrwb,libopencore_amrwb,ape,flac,vorbis,libvorbis,libopus,opus
    --enable-encoder=aac,aac_at,aac_fixed,aac_latm
    --enable-decoder=adpcm_4xm,adpcm_adx,adpcm_afc,adpcm_aica,adpcm_ct,adpcm_dtk,adpcm_g722,adpcm_g726,adpcm_g726le
    --enable-decoder=adpcm_ima_amv,adpcm_ima_apc,adpcm_ima_iss,adpcm_ima_oki,adpcm_ima_rad
    --enable-decoder=adpcm_ima_wav,adpcm_psx,adpcm_sbpro_2,adpcm_sbpro_3,adpcm_sbpro_4,adpcm_thp,adpcm_thp_le,adpcm_xa,adpcm_yamaha
    --enable-decoder=g723_1,g729,gsm,libgsm
    --enable-decoder=dsd_lsbf,dsd_msbf,dsd_msbf_planar,dsd_lsbf_planar
    --enable-muxer=ac3,rtsp,matroska,matroska_audio,mjpeg,mlp,mmf,mov,mp2,mp4,mpeg1system,mpeg1vcd,mpeg1video,mpeg2dvd,mpeg2svcd,mpeg2video,mpeg2vob,oga,ogg,ogv,opus,psp,rawvideo,sbc,segment,stream_segment,singlejpeg,spx,swf,tg2,tgp,truehd,vc1,w64,wav,webm,webm_dash_manifest,webm_chunk
    --enable-demuxer=ac3,eac3,adf,sdf,img2,rtpdec,img,rtp,amrnb,amrwb,aptx,aptx_hd,vcl,au,bintext,bit,codec2,codec2raw,data,g726,g726le,idf,image2,image2pipe,mlp,mmf,truehd,v210,v210x,vobsub,w64,xbin,vc1
    --enable-decoder=rtpdec,rtp,g726_16_dynamic,g726_24_dynamic,g726_32_dynamic,g726_40_dynamic,g7261e_16_dynamic,g7261e_24_dynamic,g726le_32_dynamic,g726le_40_dynamic,amv,asv1,asv2,aura,avrp,ayuv,cyuv,eightsvx_exp,eightsvx_fib,ffvhuff,hymt,iff_ilbm,mszh,mvc1,mvc2,pam,pbm,pgm,pgmyuv,ppm,r10k,r210,theora,thp,v408,vc1image,vp4,vp7,wmv3,wmv3image,aptx,aptx_hd,eac3,gsm_ms,iac,imc,mlp,sonic,truehd,wmav1,wmav2,pcm_f16le,pcm_f24le,pcm_s8,pcm_s8_planar,pcm_24be,pcm_s24daud,pcm_s24be,pcm_s24le,pcm_s24le_planar,pcm_s32be,pcm_s32le,pcm_s32le_planar,pcm_s64be,pcm_s64le,pcm_u8,pcm_u16be,pcm_u16le,pcm_u24be,pcm_u24le,pcm_u32be,pcm_u32le,pcm_vidc,ssa,ass,pjs,srt,stl,subrip,subviewer1,text,vplayer,bintext,xbin,idf,vp5,vp6,dirac,diracdsp
    --enable-encoder=asv1,asv2,avrp,ayuv,ffvhuff,mjpeg,mpeg4,opus,pbm,pgm,pgmyuv,ppm,r10k,r210,v408,aptx,aptx_hd,mlp,sonic,sonic_ls,truehd,wmav1,wmav2,pcm_alaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_mulaw,s8,s8_planar,pcm_s16be,pcm_s16be_planar,pcm_s24be,pcm_s24daud,pcm_s24le,pcm_s24le_planar,pcm_s32be,pcm_s32le,pcm_s32le_planar,adpcm_g726le,dvvideo,dvaudio,h263
    --enable-parser=rv30,rv40
"

FF_CONFIG_OPTIONS=`echo $FF_CONFIG_OPTIONS`

${FFMPEG_PATH}/configure ${FF_CONFIG_OPTIONS}
sed -i 's/HAVE_SYSCTL 1/HAVE_SYSCTL 0/g' config.h
sed -i 's/HAVE_SYSCTL=yes/!HAVE_SYSCTL=yes/g' ./ffbuild/config.mak
sed -i 's/HAVE_GLOB 1/HAVE_GLOB 0/g' config.h
sed -i 's/HAVE_GLOB=yes/!HAVE_GLOB=yes/g' config.h
sed -i 's/HAVE_GMTIME_R 1/HAVE_GMTIME_R 0/g' config.h
sed -i 's/HAVE_LOCALTIME_R 1/HAVE_LOCALTIME_R 0/g' config.h
sed -i 's/HAVE_PTHREAD_CANCEL 1/HAVE_PTHREAD_CANCEL 0/g' config.h
sed -i 's/HAVE_VALGRIND_VALGRIND_H 1/HAVE_VALGRIND_VALGRIND_H 0/g' config.h

tmp_file=".tmpfile"
## remove invalid restrict define
sed 's/#define av_restrict restrict/#define av_restrict/' ./config.h >$tmp_file
mv $tmp_file ./config.h

## replace original FFMPEG_CONFIGURATION define with $FF_CONFIG_OPTIONS
sed '/^#define FFMPEG_CONFIGURATION/d' ./config.h >$tmp_file
mv $tmp_file ./config.h
total_line=`wc -l ./config.h | cut -d' ' -f 1`
tail_line=`expr $total_line - 3`
head -3 config.h > $tmp_file
echo "#define FFMPEG_CONFIGURATION \"${FF_CONFIG_OPTIONS}\"" >> $tmp_file
tail -$tail_line config.h >> $tmp_file
mv $tmp_file ./config.h

rm -f config.err

## rm BUILD_ROOT information
sed '/^BUILD_ROOT=/d' ./ffbuild/config.mak > $tmp_file
rm -f ./ffbuild/config.mak
mv $tmp_file ./ffbuild/config.mak

## rm amr-eabi-gcc
sed '/^CC=arm-eabi-gcc/d' ./ffbuild/config.mak > $tmp_file
rm -f ./ffbuild/config.mak
mv $tmp_file ./ffbuild/config.mak

## rm amr-eabi-gcc
sed '/^AS=arm-eabi-gcc/d' ./ffbuild/config.mak > $tmp_file
rm -f ./ffbuild/config.mak
mv $tmp_file ./ffbuild/config.mak


## rm amr-eabi-gcc
sed '/^LD=arm-eabi-gcc/d' ./ffbuild/config.mak > $tmp_file
rm -f ./ffbuild/config.mak
mv $tmp_file ./ffbuild/config.mak

## rm amr-eabi-gcc
sed '/^DEPCC=arm-eabi-gcc/d' ./ffbuild/config.mak > $tmp_file
rm -f ./ffbuild/config.mak
mv $tmp_file ./ffbuild/config.mak

sed -i 's/restrict restrict/restrict /g' config.h

sed -i '/getenv(x)/d' config.h
sed -i 's/HAVE_DOS_PATHS 0/HAVE_DOS_PATHS 1/g' config.h

mv config.h ${FFMPEG_OUT_PATH}/config.h
mv ./ffbuild/config.mak ${FFMPEG_OUT_PATH}/config.mak
mv -f libavcodec ${FFMPEG_OUT_PATH}
mv -f libavformat ${FFMPEG_OUT_PATH}
mv -f libavutil ${FFMPEG_OUT_PATH}
mv -f libavdevice ${FFMPEG_OUT_PATH}
mv -f libavfilter ${FFMPEG_OUT_PATH}
rm -rf ./ffbuild

## other work need to be done manually
cat <<!EOF
#####################################################
                    ****NOTICE****
You need to modify the file config.mak and delete
all full path string in macro:
SRC_PATH, SRC_PATH_BARE, BUILD_ROOT, LDFLAGS.
Please refer to the old version of config.mak to
check how to modify it.
#####################################################
!EOF
