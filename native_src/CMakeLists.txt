cmake_minimum_required(VERSION 3.10)
project(media_app LANGUAGES C CXX)


# add_library(media_app SHARED
#     src/audio_player.hpp
#     src/audio_player.cpp
#     src/audio_device.hpp
#     src/audio_device.cpp
#     src/ffi_wrapper.h
#     src/ffi_wrapper.cpp
# )

add_executable(media_app
    src/main.cpp
    src/audio_player.hpp
    src/audio_device.hpp
    src/audio_device.cpp
    src/ffi_wrapper.h
    src/ffi_wrapper.cpp
    src/audio_file_saver.cpp
    src/audio_file_saver.hpp
    src/audio_decoder.hpp
    src/audio_decoder.cpp
)

target_include_directories(media_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_directories(media_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(media_app PRIVATE
    -Wl,--start-group
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
