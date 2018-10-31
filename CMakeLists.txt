# Copyright 2018, alex at staticlibs.net
# Copyright 2018, mike at myasnikov.mike@gmail.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required ( VERSION 2.8.12 )

project ( wilton_video_handler CXX )

# dependencies
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" )
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" )
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
set (PLATFORM_INCLUDE "")
if ( NOT DEFINED ARCHITECTURE)
  set (ARCHITECTURE "x64") # or x86
endif ()

if(EXISTS "/etc/redhat-release")
     set ( PLATFORM_INCLUDE "/usr/include/ffmpeg")
endif(EXISTS "/etc/redhat-release")

set (JANSSON_BUILD_DOCS OFF CACHE BOOL "Disable jansson build docs")
set (JANSSON_EXAMPLES OFF CACHE BOOL "Disable jansson build examples")
set (JANSSON_WITHOUT_TESTS ON CACHE BOOL "Disable jansson build tests")
include_directories(jansson/include)
link_directories(jansson/lib)
add_subdirectory(jansson)

set(SOURCES
    src/frame_keeper.cpp
    src/recognizer.cpp
    src/decoder.cpp
    src/encoder.cpp
    src/display.cpp
    src/photo.cpp
    src/utils.cpp
    src/video_handler.cpp)

set ( LIBPATH  "/usr/local/lib" )

set(LIBS 
    avformat
    avcodec
    swscale
    avdevice
    swresample
    avutil
    SDL2
    pthread
    jansson
    ${LIBPATH}/libopencv_core.so
    ${LIBPATH}/libopencv_imgproc.so
    ${LIBPATH}/libopencv_objdetect.so
    ${LIBPATH}/libopencv_highgui.so
)

if ( WIN32 )
  set(LIBS 
    "$ENV{FFMPEG_DEV_DIR}/lib/avformat.lib"
    "$ENV{FFMPEG_DEV_DIR}/lib/avcodec.lib"
    "$ENV{FFMPEG_DEV_DIR}/lib/swscale.lib"
    "$ENV{FFMPEG_DEV_DIR}/lib/avdevice.lib"
    "$ENV{FFMPEG_DEV_DIR}/lib/swresample.lib"
    "$ENV{FFMPEG_DEV_DIR}/lib/avutil.lib"
    "$ENV{SDL_DEV_DIR}/lib/${ARCHITECTURE}/SDL2.lib"
    "${CMAKE_CURRENT_BINARY_DIR}/jansson/lib/Release/jansson.lib"
  )
  # needed for some MSVC installations
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /FORCE:Multiple")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO /FORCE:Multiple")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /FORCE:Multiple")
endif ( )

set(STATICLIBS_HEADERS_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/jansson/include"
    "$ENV{FFMPEG_DEV_DIR}/include"
    "$ENV{SDL_DEV_DIR}/include"
  )

message(STATUS ${LIBS})
message(STATUS ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS ${STATICLIBS_HEADERS_DIRS})

if ( WIN32 )
message(STATUS "Directory '$ENV{SDL_DEV_DIR}/include' expected to contain headers in form like: '$ENV{SDL_DEV_DIR}/include/SDL2/SDL.h'")
message(STATUS "This configuration simplify build on Linux systems")
endif ( )

add_library ( ${PROJECT_NAME} SHARED
        ${SOURCES} )

target_include_directories (${PROJECT_NAME} BEFORE PRIVATE
        $ENV{WILTON_HOME}/devel/include
        ${PLATFORM_INCLUDE}
        ${STATICLIBS_HEADERS_DIRS})

if ( WIN32 )
    target_link_libraries ( ${PROJECT_NAME} PRIVATE
            $ENV{WILTON_HOME}/devel/wilton_core.lib )
    set (WIN_CUSTOM_COMMAND_1 
      COMMAND ${CMAKE_COMMAND} -E copy_directory $ENV{FFMPEG_DEV_DIR}/bin dist/bin)
    set (WIN_CUSTOM_COMMAND_2
      COMMAND ${CMAKE_COMMAND} -E copy_directory $ENV{SDL_DEV_DIR}/lib/${ARCHITECTURE} dist/bin)
endif ( )

target_link_libraries ( ${PROJECT_NAME} PRIVATE ${LIBS})

add_custom_target ( dist ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory dist
        COMMAND ${CMAKE_COMMAND} -E make_directory dist
        # copy wilton libs to app bundle
        COMMAND ${CMAKE_COMMAND} -E copy_directory $ENV{WILTON_HOME}/bin dist/bin

        ${WIN_CUSTOM_COMMAND_1}
        ${WIN_CUSTOM_COMMAND_2}

        # copy example module
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}
            dist/bin/
        # copy JS stdlib
        COMMAND ${CMAKE_COMMAND} -E copy $ENV{WILTON_HOME}/std.wlib dist/
        # copy JS app
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/index.js dist/index.js
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${PROJECT_NAME}
        COMMENT "Preparing application bundle, directory: [${CMAKE_CURRENT_BINARY_DIR}/dist] ..." )

add_dependencies(dist jansson)
