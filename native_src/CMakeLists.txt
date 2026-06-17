cmake_minimum_required(VERSION 3.10)
project(media_app LANGUAGES C CXX)

# SDL2 ကို ရှာခိုင်းခြင်း
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS}
    src
    src/core
    src/audio
    src/video
    src/utils
    src/ffi
)

# add_library(media_app SHARED
#     src/audio_player.hpp
#     src/audio_device.hpp
#     src/audio_device.cpp
#     src/ffi_wrapper.h
#     src/ffi_wrapper.cpp
#     src/audio_file_saver.cpp
#     src/audio_file_saver.hpp
#     src/media_file.hpp
#     src/media_file.cpp
#     src/video_reader.cpp
#     src/video_reader.hpp
# )

add_executable(media_app
    src/main.cpp
    src/audio/audio_player.hpp
    src/audio/audio_device.hpp
    src/audio/audio_device.cpp
    src/ffi/ffi_wrapper.h
    src/ffi/ffi_wrapper.cpp
    src/audio/audio_file_saver.cpp
    src/audio/audio_file_saver.hpp
    src/core/media_file.hpp
    src/core/media_file.cpp
    src/video/video_reader.cpp
    src/video/video_reader.hpp
    src/ffi/ffi_audio_wrapper.cpp

)

target_include_directories(media_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/src/video
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ffi
)

target_link_directories(media_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(media_app PRIVATE
    -Wl,--start-group
    ${SDL2_LIBRARIES}
    avdevice
    avfilter
    avformat
    avcodec
    swresample
    swscale
    avutil
    -Wl,--end-group
    pthread
    m
    z
    lzma
    bz2
    drm
)
