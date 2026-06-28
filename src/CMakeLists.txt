cmake_minimum_required(VERSION 3.10)
project(than_media LANGUAGES C CXX)


# 1. Android NDK အတွက် C++ Runtime ကို Static Link လုပ်ခိုင်းခြင်း (libc++_shared.so မလိုတော့အောင်)
if(ANDROID)
    set(CMAKE_ANDROID_STL_TYPE "c++_static")
endif()
# 🎯 SDL2 ကို Linux အတွက်ပဲ ရှာခိုင်းရန် (Android build ကို မထိခိုက်စေရန်)
if(NOT ANDROID)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

include_directories(
    core
    audio
    video
    utils
    ffi
)

add_library(than_media SHARED
    imple.cpp
    audio/audio_player.hpp
    audio/audio_device.hpp
    audio/audio_device.cpp
    ffi/ffi_wrapper.h
    ffi/ffi_wrapper.cpp
    audio/audio_file_saver.cpp
    audio/audio_file_saver.hpp
    core/media_file.hpp
    core/media_file.cpp
    video/video_reader.cpp
    video/video_reader.hpp
    ffi/ffi_audio_wrapper.cpp
    ffi/ffi_file_saver.cpp
    ffi/media_file_wrapper.cpp

)

# add_executable(than_media
#     main.cpp
#     imple.cpp
#     audio/audio_player.hpp
#     audio/audio_device.hpp
#     audio/audio_device.cpp
#     ffi/ffi_wrapper.h
#     ffi/ffi_wrapper.cpp
#     audio/audio_file_saver.cpp
#     audio/audio_file_saver.hpp
#     core/media_file.hpp
#     core/media_file.cpp
#     video/video_reader.cpp
#     video/video_reader.hpp
#     ffi/ffi_audio_wrapper.cpp
#     ffi/ffi_file_saver.cpp
#     ffi/media_file_wrapper.cpp


# )

target_include_directories(than_media PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/ffi
    ${CMAKE_CURRENT_SOURCE_DIR}/video
)

if(LINUX)
    # Linux Desktop အတွက်
    target_link_libraries(than_media PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/linux_x64/libffmpeg.so
        ${SDL2_LIBRARIES}
        X11 Xv xcb # X11, XVideo နှင့် core XCB
        xcb-shm xcb-xfixes xcb-shape # XCB sub-modules (အသစ်ဖြည့်စွက်ချက်)
        asound sndio # ALSA နှင့် Sndio Audio (အသစ်ဖြည့်စွက်ချက်)
        vdpau # VDPAU Hardware Acceleration (အသစ်ဖြည့်စွက်ချက်)
        lzma bz2 z # Compression 
        va va-drm va-x11 # VAAPI
        pthread dl # Core system threads
    )

    # Target ရဲ့ Type ကို စစ်ဆေးခြင်း
    get_target_property(TARGET_TYPE than_media TYPE)

    if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        message(STATUS "Configuring RPATH for Shared Library")
        set_target_properties(than_media PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endif()


# ၄။ OS အလိုက် သက်ဆိုင်ရာ Library များနှင့် Graphic APIs များ ခွဲခြား Link လုပ်ခြင်း
if(ANDROID)
    # Android အတွက် (ဥပမာ - build/toolchain ကပေးမယ့် ANDROID_ABI အလိုက် arm64-v8a စသဖြင့် ခွဲချင်ရင်)
    # လောလောဆယ် သင့် folder structure က lib/android/arm64-v8a/ အောက်မှာ ရှိတယ်ဆိုပါစို့
    target_link_libraries(than_media PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/android/${ANDROID_ABI}/libffmpeg.so
        log # Android native logcat သုံးနိုင်ရန် (လိုအပ်လျှင်)
        android # Android native app APIs

    )
endif()

