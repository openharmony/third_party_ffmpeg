#!/bin/bash

set -ex
FFMPEG_PATH=$1
FFMPEG_OUT_PATH=$2
FFMPEG_PLAT=$3
LLVM_PATH=$4
SYSROOT_PATH=$5
USE_CLANG_COVERAGE=$6

oldPath=`pwd`
FFMPEG_PATH=${oldPath}/${FFMPEG_PATH}
FFMPEG_OUT_PATH=${oldPath}/${FFMPEG_OUT_PATH}
currentPath=${FFMPEG_OUT_PATH}tmp
mkdir -p ${currentPath}
cd ${currentPath}

FF_CONFIG_OPTIONS="
    --target-os=mingw64
    --arch=x86_64
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
    --disable-asm
    --disable-doc
    --disable-debug
    --disable-iconv
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
"

FF_CONFIG_OPTIONS=`echo $FF_CONFIG_OPTIONS`

${FFMPEG_PATH}/configure ${FF_CONFIG_OPTIONS}

# Apply necessary patches to the configuration for MinGW/Windows

# Disable system-specific features for Windows
sed -i 's/HAVE_GMTIME_R 1/HAVE_GMTIME_R 0/g' config.h
sed -i 's/HAVE_LOCALTIME_R 1/HAVE_LOCALTIME_R 0/g' config.h
sed -i 's/HAVE_PTHREAD_CANCEL 1/HAVE_PTHREAD_CANCEL 0/g' config.h
sed -i 's/HAVE_SYSCTL 1/HAVE_SYSCTL 0/g' config.h
sed -i 's/HAVE_SYSCTL=yes/!HAVE_SYSCTL=yes/g' ./ffbuild/config.mak
sed -i 's/HAVE_VALGRIND_VALGRIND_H 1/HAVE_VALGRIND_VALGRIND_H 0/g' config.h
sed -i 's/restrict restrict/restrict /g' config.h

sed -i 's/HAVE_CBRT 0/HAVE_CBRT 1/g' config.h
sed -i 's/HAVE_CBRTF 0/HAVE_CBRTF 1/g' config.h
sed -i 's/HAVE_COPYSIGN 0/HAVE_COPYSIGN 1/g' config.h
sed -i 's/HAVE_ERF 0/HAVE_ERF 1/g' config.h
sed -i 's/HAVE_HYPOT 0/HAVE_HYPOT 1/g' config.h
sed -i 's/HAVE_RINT 0/HAVE_RINT 1/g' config.h
sed -i 's/HAVE_LRINT 0/HAVE_LRINT 1/g' config.h
sed -i 's/HAVE_LRINTF 0/HAVE_LRINTF 1/g' config.h
sed -i 's/HAVE_ROUND 0/HAVE_ROUND 1/g' config.h
sed -i 's/HAVE_ROUNDF 0/HAVE_ROUNDF 1/g' config.h
sed -i 's/HAVE_TRUNC 0/HAVE_TRUNC 1/g' config.h
sed -i 's/HAVE_TRUNCF 0/HAVE_TRUNCF 1/g' config.h

# Remove invalid restrict defines
tmp_file=".tmpfile"
sed 's/#define av_restrict restrict/#define av_restrict/' ./config.h >$tmp_file
mv $tmp_file ./config.h

# Fix for DOS paths on Windows
sed -i 's/HAVE_DOS_PATHS 0/HAVE_DOS_PATHS 1/g' config.h

# Fix getenv issue on Windows
sed -i '/getenv(x)/d' config.h

# Copy configuration files to output path
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

## Manual steps notice
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
