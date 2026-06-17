#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

// အသံရဲ့ Format Information တွေကို တခြား Class တွေက လှမ်းသိနိုင်ဖို့ Struct တစ်ခု ဆောက်ထားမယ်
struct AudioFormatInfo {
  int sampleRate = 44100;
  int channels = 2;
  AVSampleFormat sampleFmt = AV_SAMPLE_FMT_S16;  // miniaudio နဲ့ အဆင်ပြေဆုံး format
};
class AudioDecoder {
 public:
  AudioDecoder(const std::string& video_path,
               const AudioFormatInfo& target_format = AudioFormatInfo());
  ~AudioDecoder();

  // ၁။ အစောကြီးကတည်းက conversion ကြီး တစ်ခုလုံး လုပ်ပစ်တာမျိုး မဟုတ်တော့ဘူး
  // ဒီကောင်က ဖိုင်ပွင့်မပွင့်နဲ့ Audio Stream ပါမပါပဲ စစ်ပြီး Context တွေ ပြင်ဆင်မယ်
  bool openFile();

  // ၂။ အခြား Class တွေ (Player/Saver) ကနေ ကွင်းဆက်မပြတ် (Loop ပတ်ပြီး) PCM data တောင်းဖို့
  // သုံးမယ့် function ဒီကောင်က ဗီဒီယိုထဲကနေ Packet တစ်ခုချင်းစီကို Decode/Resample လုပ်ပြီး RAW
  // bytes တွေကို chunk လိုက် ထုတ်ပေးမယ် ဖိုင်ဆုံးသွားရင် false ပြန်မယ်
  bool readNextAudioChunk(std::vector<uint8_t>& out_chunk);
  bool seekToSeconds(double seconds);
  double getDurationInSeconds();
  double getCurrentInSeconds();

  // ၃။ တခြား Class တွေက လာမေးမယ့် Getter Functions
  AudioFormatInfo getTargetFormat() const { return targetFormat; }
  std::string getCurrentPath() const { return inputPath; }
  void closeFile();

 private:
  std::string inputPath;
  double currentTimestampSeconds = 0.0;

  // FFmpeg Contexts
  AVFormatContext* formatContext = nullptr;
  AVCodecContext* codecContext = nullptr;
  SwrContext* swrCtx = nullptr;
  int audioStreamIndex = -1;

  // FFmpeg Internal Reusable Packets/Frames
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;

  // Target Audio Format Configuration
  AudioFormatInfo targetFormat;

  bool initResampler();
};