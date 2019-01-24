cmake_minimum_required(VERSION 3.4.1)

set(
	PATH_TO_SUPERPOWERED
	PATH_TO_LAME
	PATH_TO_LIB
	CACHE STRING ""
)

message(${ANDROID_ABI})

include_directories(${PATH_TO_LIB}/src/main/jni)
include_directories(${PATH_TO_SUPERPOWERED})
include_directories(${PATH_TO_LAME})

add_library(
	sp-wrapper-lib
	SHARED
	${PATH_TO_LIB}/src/main/jni/_interface.cpp
	${PATH_TO_LIB}/src/main/jni/SPPlayer.cpp
	${PATH_TO_LIB}/src/main/jni/SPRecorder.cpp
	${PATH_TO_LIB}/src/main/jni/base/SPSharedVariables.cpp
	${PATH_TO_SUPERPOWERED}/AndroidIO/SuperpoweredAndroidAudioIO.cpp
	${PATH_TO_LAME}/lame.c ${PATH_TO_LAME}/set_get.c ${PATH_TO_LAME}/util.c ${PATH_TO_LAME}/gain_analysis.c
	${PATH_TO_LAME}/bitstream.c ${PATH_TO_LAME}/presets.c ${PATH_TO_LAME}/tables.c ${PATH_TO_LAME}/quantize_pvt.c
	${PATH_TO_LAME}/psymodel.c ${PATH_TO_LAME}/id3tag.c ${PATH_TO_LAME}/encoder.c ${PATH_TO_LAME}/fft.c
    ${PATH_TO_LAME}/mpglib_interface.c ${PATH_TO_LAME}/newmdct.c ${PATH_TO_LAME}/quantize.c ${PATH_TO_LAME}/reservoir.c
    ${PATH_TO_LAME}/takehiro.c ${PATH_TO_LAME}/vbrquantize.c ${PATH_TO_LAME}/VbrTag.c ${PATH_TO_LAME}/version.c
)

target_link_libraries(
	sp-wrapper-lib
    log
    android
    OpenSLES
    ${PATH_TO_SUPERPOWERED}/libSuperpoweredAndroid${ANDROID_ABI}.a
)