#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

// အသံရဲ့ Format Information တွေကို တခြား Class တွေက လှမ်းသိနိုင်ဖို့ Struct တစ်ခု ဆောက်ထားမယ်
struct AudioFormatInfo {
  int sampleRate = 44100;
  int channels = 2;
  AVSampleFormat sampleFmt = AV_SAMPLE_FMT_S16;  // miniaudio နဲ့ အဆင်ပြေဆုံး format
};

class MediaFile {
 public:
  MediaFile(const std::string& video_path,
            const AudioFormatInfo& target_format = AudioFormatInfo());
  ~MediaFile();
  bool openFile();
  bool readNextAudioChunk(std::vector<uint8_t>& out_chunk);
  bool seekToSeconds(double seconds);
  double getDurationInSeconds();
  double getCurrentInSeconds();
  std::string getMetadata(const std::string& key);
  std::vector<uint8_t> getAlbumArt();

  // ၃။ တခြား Class တွေက လာမေးမယ့် Getter Functions
  AudioFormatInfo getTargetFormat() const { return targetFormat; }
  std::string getCurrentPath() const { return inputPath; }
  void closeFile();
  //**********video********** */
  // targetWidth နဲ့ targetHeight ကို ၀ ထားရင် မူရင်း size အတိုင်း ထွက်ပါမယ်
  std::vector<uint8_t> getVideoThumbnail(double seconds, int targetWidth = 0,
                                         int targetHeight = 0);
  bool saveAsVideoThumbnail(const std::string& outPath, double seconds,
                            int targetWidth = 0, int targetHeight = 0);
  // VideoFormatInfo getVideoFormatInfo();

 private:
  std::string inputPath;
  double currentTimestampSeconds = 0.0;

  // FFmpeg Contexts
  AVFormatContext* formatContext = nullptr;
  AVCodecContext* codecContext = nullptr;
  SwrContext* swrCtx = nullptr;
  int audioStreamIndex = -1;
  int videoStreamIndex = -1;
  AVCodecContext* videoCodecContext = nullptr;

  // FFmpeg Internal Reusable Packets/Frames
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;

  // Target Audio Format Configuration
  AudioFormatInfo targetFormat;

  bool initResampler();
};