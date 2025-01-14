# FFmpeg build configure

#!/bin/bash

set -ex
FFMPEG_PATH=$1
FFMPEG_OUT_PATH=$2
FFMPEG_PLAT=$3

oldPath=`pwd`
FFMPEG_PATH=${oldPath}/${FFMPEG_PATH}
FFMPEG_OUT_PATH=${oldPath}/${FFMPEG_OUT_PATH}
currentPath=${FFMPEG_OUT_PATH}tmp
mkdir -p ${currentPath}
cd ${currentPath}

FF_CONFIG_OPTIONS=""

if [ ${FFMPEG_PLAT} = "aarch64" ]; then
FF_CONFIG_OPTIONS="
    --arch=arm64
"
else
FF_CONFIG_OPTIONS="
    --arch=x86_64
    --disable-asm
"
fi

FF_CONFIG_OPTIONS+="
    --target-os=darwin
    --disable-programs
    --disable-avdevice
    --disable-postproc
    --disable-avfilter
    --disable-network
    --disable-dwt
    --disable-faan
    --disable-pixelutils
    --disable-bsfs
    --disable-encoders
    --disable-decoders
    --disable-hwaccels
    --disable-muxers
    --disable-demuxers
    --disable-parsers
    --disable-protocols
    --disable-devices
    --disable-filters
    --disable-doc
    --disable-debug
    --enable-iconv
    --disable-stripping
    --disable-vaapi
    --disable-vdpau
    --disable-zlib
    --disable-xlib
    --disable-cuvid
    --disable-cuda
    --disable-libxcb
    --disable-libxcb_shm
    --disable-libxcb_shape
    --disable-libxcb_xfixes
    --disable-sdl2
    --disable-bzlib
    --disable-lzma
    --disable-videotoolbox
    --enable-demuxer=mp3,aac,ape,flac,ogg,wav,mov,mpegts,amr,amrnb,amrwb,matroska,flv,mpegps
    --enable-muxer=mp4,h264,ipod,amr
    --enable-parser=h263,h264,mpeg4video,vp8,vp9,mpegvideo
    --enable-parser=mpegaudio,aac,aac_latm,av3a,amr
    --enable-decoder=h263,h264,mpeg2video,mpeg4,vp8,vp9
    --enable-decoder=mp3,mp3float,aac,aac_latm,ape,flac,vorbis,opus,amrnb,amrwb
    --enable-decoder=png,bmp
    --enable-encoder=aac,aac_latm,opus,flac
    --enable-encoder=mpeg4,h263
    --enable-bsf=h264_mp4toannexb
    --enable-protocol=file
    --enable-lsp
    --enable-shared
    --enable-cross-compile
    --enable-neon
    --enable-armv8
"

FF_CONFIG_OPTIONS=`echo $FF_CONFIG_OPTIONS`

${FFMPEG_PATH}/configure ${FF_CONFIG_OPTIONS}

sed -i '' 's/HAVE_GMTIME_R 1/HAVE_GMTIME_R 0/g' config.h
sed -i '' 's/HAVE_LOCALTIME_R 1/HAVE_LOCALTIME_R 0/g' config.h
sed -i '' 's/HAVE_PTHREAD_CANCEL 1/HAVE_PTHREAD_CANCEL 0/g' config.h
sed -i '' 's/HAVE_SYSCTL 1/HAVE_SYSCTL 0/g' config.h
sed -i '' 's/HAVE_SYSCTL=yes/!HAVE_SYSCTL=yes/g' ./ffbuild/config.mak
sed -i '' 's/HAVE_VALGRIND_VALGRIND_H 1/HAVE_VALGRIND_VALGRIND_H 0/g' config.h
sed -i '' 's/restrict restrict/restrict /g' config.h

## remove invalid restrict define
tmp_file=".tmpfile"
sed 's/#define av_restrict restrict/#define av_restrict/' ./config.h >$tmp_file
mv $tmp_file ./config.h

sed -i '' '/getenv(x)/d' config.h
sed -i '' 's/HAVE_DOS_PATHS 0/HAVE_DOS_PATHS 1/g' config.h

mv config.h ${FFMPEG_OUT_PATH}/config.h
mv ./ffbuild/config.mak ${FFMPEG_OUT_PATH}/config.mak
rm -rf ${FFMPEG_OUT_PATH}/libavcodec
mv -f libavcodec ${FFMPEG_OUT_PATH}
rm -rf ${FFMPEG_OUT_PATH}/libavformat
mv -f libavformat ${FFMPEG_OUT_PATH}
rm -rf ${FFMPEG_OUT_PATH}/libavutil
mv -f libavutil ${FFMPEG_OUT_PATH}
rm -rf ${FFMPEG_OUT_PATH}/libavdevice
mv -f libavdevice ${FFMPEG_OUT_PATH}
rm -rf ${FFMPEG_OUT_PATH}/libavfilter
mv -f libavfilter ${FFMPEG_OUT_PATH}
rm -rf ./ffbuild

cd $oldPath
rm -rf ${currentPath}

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
