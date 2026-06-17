#pragma once

#include <string>

#include "audio_decoder.hpp"
extern "C" {
#include <libavutil/audio_fifo.h>
}
class AudioFileSaver {
 public:
  // WAV ဖိုင်အဖြစ် သိမ်းဆည်းမည့် Function
  static bool saveAsWav(AudioDecoder& decoder, const std::string& outPath);

  static bool saveAsAac(AudioDecoder& decoder, const std::string& outPath);

  // MP3 ဖိုင်အဖြစ် သိမ်းဆည်းမည့် Function
  static bool saveAsMp3(AudioDecoder& decoder, const std::string& outPath);

 private:
  // WAV Header ရေးရန် Helper
  static void writeWavHeader(std::ofstream& file, int sampleRate, int channels,
                             int totalDataSize);
};