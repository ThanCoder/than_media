#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>             // ထပ်တိုး
#include <libswresample/swresample.h>  // ထပ်တိုး

#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
}

class VideoAudioConverter {
 public:
  VideoAudioConverter(const std::string& video_path,
                      const std::string& out_path);
  ~VideoAudioConverter();
  bool convert();

 private:
  std::string inputPath;
  std::string outputPath;

  AVFormatContext* formatContext = nullptr;
  AVCodecContext* codecContext = nullptr;
  SwrContext* swrCtx = nullptr;  // ထပ်တိုး Resampler Context
  int audioStreamIndex = -1;
  std::ofstream outputFile;

  // Target format အသေ သတ်မှတ်ချက် (Standard CD Quality WAV)
  const int TARGET_SAMPLE_RATE = 44100;
  const int TARGET_CHANNELS = 2;
  const AVSampleFormat TARGET_SAMPLE_FMT =
      AV_SAMPLE_FMT_S16;  // 16-bit signed integer

  bool initialize();
  void cleanUp();
  int decodePacket(AVPacket* av_pocket, AVFrame* av_frame);

  // mini audio အတွက်
  std::vector<uint8_t> audioBuffer;

 public:
  // miniaudio က လှမ်းယူမည့် Getter Functions
  const std::vector<uint8_t>& GetAudioBuffer() const { return audioBuffer; }
  int GetSampleRate() const { return TARGET_SAMPLE_RATE; }
  int GetChannels() const { return TARGET_CHANNELS; }
};
