#include <string>

#include "audio_file_saver.hpp"
#include "ffi_wrapper.h"
#include "media_file.hpp"

bool file_saver_save_as(const char* video_path, const char* out_path,
                        int format, OnProgressCallback on_progress) {
  if (!video_path || !out_path) return false;
  std::string videoPath(video_path);
  std::string outPath(out_path);
  MediaFile mf{videoPath};

  AudioFileSaver sf;
  switch (format) {
    case AUDIO_FORMAT_WAV:
      return sf.saveAsWav(mf, outPath, on_progress);
    case AUDIO_FORMAT_AAC:
      return sf.saveAsAac(mf, outPath, on_progress);
    case AUDIO_FORMAT_MP3:
      return sf.saveAsMp3(mf, outPath, on_progress);
    default:
      return false;
  }
}

bool file_saver_saveAsWav(void* media_file_ptr, const char* outPath) {
  auto mf = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (!mf) return false;
  AudioFileSaver sf;
  return sf.saveAsWav(*mf, outPath);
}
bool file_saver_saveAsAac(void* media_file_ptr, const char* outPath) {
  auto mf = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (!mf) return false;
  AudioFileSaver sf;
  return sf.saveAsAac(*mf, outPath);
}
bool file_saver_saveAsMp3(void* media_file_ptr, const char* outPath) {
  auto mf = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (!mf) return false;
  AudioFileSaver sf;
  return sf.saveAsMp3(*mf, outPath);
}