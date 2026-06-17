#include "ffi_wrapper.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include "video_reader.hpp"

void* video_reader_create(const char* video_path) {
  auto reader = new VideoReader(video_path);
  if (reader->openFile()) {
    return reader;
  }
  delete reader;
  return nullptr;
}
bool video_reader_decode_next_frame(void* reader,
                                    unsigned char* output_buffer) {
  if (!reader || !output_buffer) return false;
  std::vector<uint8_t> pixel_cache;
  auto re = reinterpret_cast<VideoReader*>(reader);

  if (re->readNextFrame(pixel_cache)) {
    std::memcpy(output_buffer, pixel_cache.data(), pixel_cache.size());
    return true;
  }
  return false;
}

Video_Format_Info video_reader_get_format_info(void* reader_ptr) {
  Video_Format_Info info;
  info.height = 0;
  info.width = 0;
  info.fps = 0.0;
  info.durationSeconds = 0.0;

  auto vd = reinterpret_cast<VideoReader*>(reader_ptr);
  if (vd) {
    auto nf = vd->getFormatInfo();
    info.height = nf.height;
    info.width = nf.width;
    info.fps = nf.fps;
    info.durationSeconds = nf.durationSeconds;
  }
  return info;
}

const char* video_reader_get_codec_name(void* reader_ptr) {
  auto vd = reinterpret_cast<VideoReader*>(reader_ptr);
  if (vd) {
    return vd->getCodecCString();
  }
  return nullptr;
}
void close_video_reader(void* reader_ptr) {
  auto vd = reinterpret_cast<VideoReader*>(reader_ptr);
  if (vd) {
    vd->closeFile();
    delete vd;
  }
}
void video_reader_destroy(void* reader_ptr) {
  auto re = reinterpret_cast<VideoReader*>(reader_ptr);
  if (re) delete re;
}
