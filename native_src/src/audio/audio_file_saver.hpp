#pragma once

#include <string>

#include "media_file.hpp"
extern "C" {
#include <libavutil/audio_fifo.h>
}
class AudioFileSaver {
 public:
  // WAV ဖိုင်အဖြစ် သိမ်းဆည်းမည့် Function
  static bool saveAsWav(MediaFile& decoder, const std::string& outPath);

  static bool saveAsAac(MediaFile& decoder, const std::string& outPath);

  // MP3 ဖိုင်အဖြစ် သိမ်းဆည်းမည့် Function
  static bool saveAsMp3(MediaFile& decoder, const std::string& outPath);

 private:
  // WAV Header ရေးရန် Helper
  static void writeWavHeader(std::ofstream& file, int sampleRate, int channels,
                             int totalDataSize);
};