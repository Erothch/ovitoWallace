# - Finds FFMPEG libraries and headers
#
# Once done this will define
#  
#  FFMPEG_FOUND			- system has FFMPEG
#  FFMPEG_INCLUDE_DIR	- the include directories
#  FFMPEG_LIBRARY_DIR	- the directory containing the libraries
#  FFMPEG_LIBRARIES		- link these to use FFMPEG
#

SET(FFMPEG_HEADERS libavformat/avformat.h libavcodec/avcodec.h libavutil/avutil.h libavdevice/avdevice.h libswscale/swscale.h)

IF(WIN32)
	SET(FFMPEG_LIBRARIES libavformat.dll.a libavcodec.dll.a libavutil.dll.a libavdevice.dll.a libswscale.dll.a)
	SET(FFMPEG_LIBRARY_DIR $ENV{FFMPEGDIR}\\lib CACHE PATH "Location of the AV libraries")
	SET(FFMPEG_INCLUDE_PATHS $ENV{FFMPEGDIR}\\include CACHE PATH "Location of the AV library headers")
ELSE()
	INCLUDE(FindPkgConfig)
	IF(PKG_CONFIG_FOUND)
		PKG_CHECK_MODULES(AVFORMAT libavformat)
		PKG_CHECK_MODULES(AVCODEC libavcodec)
		PKG_CHECK_MODULES(AVUTIL libavutil)
		PKG_CHECK_MODULES(AVDEVICE libavdevice)
		PKG_CHECK_MODULES(SWSCALE libswscale)
	ENDIF()

	SET(FFMPEG_LIBRARY_DIR ${AVFORMAT_LIBRARY_DIRS}
			     ${AVCODEC_LIBRARY_DIRS}
			     ${AVUTIL_LIBRARY_DIRS}
			     ${AVDEVICE_LIBRARY_DIRS}
			     ${SWSCALE_LIBRARY_DIRS})
	SET(FFMPEG_INCLUDE_PATHS ${AVFORMAT_INCLUDE_DIRS}
			     ${AVCODEC_INCLUDE_DIRS}
			     ${AVUTIL_INCLUDE_DIRS}
			     ${AVDEVICE_INCLUDE_DIRS}
			     ${SWSCALE_INCLUDE_DIRS})

	IF(NOT APPLE)
		IF(NOT OVITO_MONOLITHIC_BUILD OR NOT UNIX)
			SET(FFMPEG_LIBRARIES avformat avcodec avutil avdevice swscale)
		ELSE()		
			SET(FFMPEG_LIBRARIES libavformat.a libavcodec.a libavutil.a libavdevice.a libswscale.a)

#			SET(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} libvorbisenc.a libx264.a 
#				libvorbis.a libogg.a libtheoraenc.a libtheoradec.a libspeex.a libschroedinger-1.0.a
#				libopus.a libmp3lame.a libopenjpeg.a liborc-0.4.a libpostproc.a
#				libswresample.a libswscale.a libxcb.a libiconv.a libvpx.a modplug z bz2)
		ENDIF()
	ELSE()
		SET(FFMPEG_LIBRARIES libavformat.a libavcodec.a libavutil.a libavdevice.a 
			libswscale.a libxvidcore.a libSDL.a libvorbisenc.a libx264.a 
			libvorbis.a libogg.a libtheoraenc.a libtheoradec.a libspeex.a libschroedinger-1.0.a
			libopus.a libmp3lame.a libopenjpeg.a liborc-0.4.a libpostproc.a
			libswresample.a libswscale.a libxcb.a libiconv.a libvpx.a modplug z bz2)
	ENDIF()
ENDIF()

# Find headers
SET(FFMPEG_FOUND TRUE)
SET(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_PATHS})
FOREACH(HEADER ${FFMPEG_HEADERS})
	SET(HEADER_PATH NOTFOUND)
	FIND_PATH(HEADER_PATH ${HEADER} PATHS ${FFMPEG_INCLUDE_PATHS} /opt/local/include $ENV{HOME}/progs/libavstatic/include)
	IF(HEADER_PATH)
		SET(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} ${HEADER_PATH})
	ELSE()
		MESSAGE("Could not locate ${HEADER}") 
		SET(FFMPEG_FOUND FALSE)
	ENDIF()
ENDFOREACH()

# Clear out duplicates
IF(NOT "${FFMPEG_INCLUDE_DIR}" MATCHES "")
	LIST(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIR)
ENDIF()
IF(NOT "${FFMPEG_LIBRARY_DIR}" MATCHES "")
	LIST(REMOVE_DUPLICATES FFMPEG_LIBRARY_DIR)
ENDIF()

IF(APPLE AND NOT FFMPEG_LIBRARY_DIR)
	SET(FFMPEG_LIBRARY_DIR "/opt/local/lib")
ENDIF()

# Find the full paths of the libraries
FOREACH(LIB ${FFMPEG_LIBRARIES})
	SET(LIB_PATH NOTFOUND)
	FIND_LIBRARY(LIB_PATH NAMES ${LIB} PATHS ${FFMPEG_LIBRARY_DIR} $ENV{HOME}/progs/libavstatic/lib)
	IF(LIB_PATH)
		SET(FFMPEG_LIBRARIES_FULL ${FFMPEG_LIBRARIES_FULL} ${LIB_PATH})
	ELSE()
		MESSAGE("Could not locate library ${LIB}") 
		SET(FFMPEG_FOUND FALSE)
	ENDIF()
ENDFOREACH()
SET(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES_FULL})

UNSET(LIB_PATH CACHE)
UNSET(HEADER_PATH CACHE)
