#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "ffi_wrapper.h"
#include "media_file.hpp"

void* media_file_create(const char* file_path) {
  return new MediaFile{file_path};
}
bool media_file_openFile(void* media_file_ptr) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    return media->openFile();
  }
  return false;
}

bool media_file_readNextAudioChunk(void* media_file_ptr,
                                   unsigned char* out_chunk) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media && out_chunk) {
    // ဥပမာ chunk size တစ်ခု သတ်မှတ်ပေးရပါမယ် (ဥပမာ- 1024 bytes)
    size_t chunk_size = 1024;

    // vector တစ်ခု ဆောက်ပြီး out_chunk ထဲက data တွေကို copy ကူးထည့်
    std::vector<uint8_t> vec(out_chunk, out_chunk + chunk_size);

    // function ကို pass လုပ်
    bool result = media->readNextAudioChunk(vec);

    // တကယ်လို့ read လုပ်ပြီး vector ထဲမှာ data အသစ်တွေ ပြောင်းသွားရင်
    // out_chunk ထဲကို ပြန်ပတ်ပေးဖို့ လိုပါတယ်
    std::copy(vec.begin(), vec.end(), out_chunk);

    return result;
  }
  return false;
}
bool media_file_seekToSeconds(void* media_file_ptr, double seconds) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    return media->seekToSeconds(seconds);
  }
  return false;
}
double media_file_getDurationInSeconds(void* media_file_ptr) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    return media->getDurationInSeconds();
  }
  return 0;
}
double media_file_getCurrentInSeconds(void* media_file_ptr) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    return media->getCurrentInSeconds();
  }
  return 0;
}
const char* media_file_getMetadata(void* media_file_ptr, const char* key) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    const std::string& data = media->getMetadata(key);
    return data.c_str();
  }
  return nullptr;
}

unsigned char* media_file_getAlbumArt(void* media_file_ptr, int* out_size) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    std::vector<uint8_t> data = media->getAlbumArt();
    if (data.empty()) return nullptr;

    size_t dataSize = data.size();
    auto buffer = static_cast<unsigned char*>(malloc(dataSize));
    if (buffer) {
      std::memcpy(buffer, data.data(), dataSize);

      if (out_size) {
        *out_size = static_cast<int>(dataSize);
      }
      return buffer;
    }
  }
  return nullptr;
}

// AudioFormatInfo getTargetFormat(void* media_file_ptr){}
const char* media_file_getCurrentPath(void* media_file_ptr) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    const std::string& path = media->getCurrentPath();
    return path.c_str();
  }
  return nullptr;
}

void media_file_closeFile(void* media_file_ptr) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    media->closeFile();
  }
}
unsigned char* media_file_getVideoThumbnail(void* media_file_ptr, int* out_size,
                                            double seconds, int targetWidth,
                                            int targetHeight) {
  auto media = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (media) {
    auto thumbnail =
        media->getVideoThumbnail(seconds, targetHeight, targetHeight);
    if (thumbnail.empty()) {
      return nullptr;
    }
    // 2. ရလာတဲ့ size အတိုင်း C-style memory အသစ် ဆောက်ပေးမယ်
    size_t data_size = thumbnail.size();
    unsigned char* buffer = static_cast<unsigned char*>(malloc(data_size));

    if (buffer) {
      // 3. vector ထဲက data တွေကို ဆောက်လိုက်တဲ့ buffer ထဲ ကူးထည့်ပေးမယ်
      memcpy(buffer, thumbnail.data(), data_size);
      if (out_size) {
        *out_size = static_cast<int>(data_size);
      }

      return buffer;
    }
  }
  return nullptr;
}
bool media_file_saveAsVideoThumbnail(const char* video_file_path,
                                     const char* out_path, double seconds,
                                     int targetWidth, int targetHeight) {
  // 1. nullptr တွေကို တစ်ခါတည်း စစ်ချပစ်တာ ပိုစိတ်ချရပါတယ်
  if (!video_file_path || !out_path) {
    return false;
  }
  std::string videoPath(video_file_path);
  auto media = MediaFile{videoPath};
  if (!media.openFile()) return false;

  std::string outPath(out_path);
  return media.saveAsVideoThumbnail(outPath, seconds, targetWidth,
                                    targetHeight);
}