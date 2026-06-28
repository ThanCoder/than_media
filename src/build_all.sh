#!/bin/bash

# အမှားတစ်ခုခုရှိလျှင် Script ကို ချက်ချင်းရပ်ပစ်ရန်
set -e

# 1. Android Paths သတ်မှတ်ခြင်း
export ANDROID_HOME=$HOME/Android/Sdk

# NDK ရှိမရှိ စစ်ဆေးခြင်း
if [ ! -d "$ANDROID_HOME/ndk" ] || [ -z "$(ls $ANDROID_HOME/ndk 2>/dev/null)" ]; then
    echo "❌ Error: Android NDK not found in $ANDROID_HOME/ndk"
    exit 1
fi

export NDK_PATH=$ANDROID_HOME/ndk/$(ls $ANDROID_HOME/ndk | head -n 1)
API_LEVEL="21"

echo "----------------------------------------"
echo "🚀 Starting Full Multi-Platform Build..."
echo "----------------------------------------"

OUTPUT_DIR="dist_binaries"
# 🎯 CMake Shared Library ရဲ့ output file name အမှန်မှာ ရှေ့က lib ပါရမည်
OUTPUT_LIB_NAME="libthan_media"

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# ========================================
# 🤖 PART 1: ANDROID BUILDS (ABI ၂ ခု)
# ========================================
ANDROID_ABIS=("arm64-v8a" "armeabi-v7a")

for ABI in "${ANDROID_ABIS[@]}"
do
    echo "📦 Building Android -> $ABI ..."
    BUILD_DIR="build_temp_android_${ABI}"
    rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
    
    cmake .. \
      -DCMAKE_SYSTEM_NAME=Android \
      -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI="$ABI" \
      -DANDROID_PLATFORM="android-$API_LEVEL" \
      -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    
    cmake --build .
    cd ..
    
    # Android Toolchain က .so ထုတ်ပေးတတ်သည့် နေရာများကို လိုက်ရှာခြင်း
    TARGET_SO=""
    for CHECK_PATH in "$BUILD_DIR/${OUTPUT_LIB_NAME}.so" "$BUILD_DIR/libs/${ABI}/${OUTPUT_LIB_NAME}.so"
    do
        if [ -f "$CHECK_PATH" ]; then
            TARGET_SO="$CHECK_PATH"
            break
        fi
    done

    ANDROID_OUT_DIR="${OUTPUT_DIR}/android/${ABI}"

    if [ -n "$TARGET_SO" ] && [ -f "$TARGET_SO" ]; then
        # 🎯 NDK ထဲက llvm-strip သို့မဟုတ် standard strip tool ကို လိုက်ရှာပြီး ခုတ်ချခြင်း
        STRIP_TOOL=$(find "$NDK_PATH/toolchains" -name "llvm-strip" -o -name "*strip" 2>/dev/null | head -n 1)
        
        if [ -n "$STRIP_TOOL" ] && [ -x "$STRIP_TOOL" ]; then
            echo "✂️ Stripping debug symbols from Android ($ABI) binary..."
            "$STRIP_TOOL" --strip-unneeded "$TARGET_SO"
        else
            # NDK strip ရှာမတွေ့ပါက Linux local strip ဖြင့် စမ်းသပ်ခုတ်ချခြင်း
            echo "⚠️ NDK Strip tool not found, using host strip..."
            strip --strip-unneeded "$TARGET_SO" 2>/dev/null || true
        fi

        mkdir -p "${ANDROID_OUT_DIR}"
        cp "$TARGET_SO" "${ANDROID_OUT_DIR}/${OUTPUT_LIB_NAME}.so"
        echo "✅ Android ($ABI) done!"
    else
        echo "❌ Android ($ABI) build failed! .so file not found."
        exit 1
    fi
    rm -rf "$BUILD_DIR"
done

# ========================================
# 🐧 PART 2: LINUX DESKTOP BUILD (x64 တစ်ခုတည်း)
# ========================================
echo "========================================"
echo "📦 Building Linux Desktop -> x64 ..."
echo "========================================"

BUILD_DIR="build_temp_linux"
rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
cmake --build .
cd ..

TARGET_SO="$BUILD_DIR/${OUTPUT_LIB_NAME}.so"
LINUX_OUT_DIR="$OUTPUT_DIR/linux"

if [ -f "$TARGET_SO" ]; then
    # Linux Desktop အတွက်လည်း local strip ဖြင့် သေချာအောင် ခုတ်ချပေးခြင်း
    echo "✂️ Stripping debug symbols from Linux (x64) binary..."
    strip --strip-unneeded "$TARGET_SO" 2>/dev/null || true

    mkdir -p "${LINUX_OUT_DIR}"
    cp "$TARGET_SO" "$LINUX_OUT_DIR/${OUTPUT_LIB_NAME}.so"
    echo "✅ Linux (x64) done!"
else
    echo "❌ Linux (x64) build failed!"
    exit 1
fi
rm -rf "$BUILD_DIR"

echo "----------------------------------------"
echo "🎉 All Platform Binaries Generated successfully!"
echo "📍 Location: $(pwd)/$OUTPUT_DIR"
echo "----------------------------------------"