#include "audio_file_saver.hpp"
#include "ffi_wrapper.h"
#include "media_file.hpp"

bool file_saver_save_as(void* media_file_ptr, const char* outPath, int format) {
  auto mf = reinterpret_cast<MediaFile*>(media_file_ptr);
  if (!mf) return false;

  AudioFileSaver sf;
  switch (format) {
    case AUDIO_FORMAT_WAV:
      return sf.saveAsWav(*mf, outPath);
    case AUDIO_FORMAT_AAC:
      return sf.saveAsAac(*mf, outPath);
    case AUDIO_FORMAT_MP3:
      return sf.saveAsMp3(*mf, outPath);
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