#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

struct VideoFormatInfo {
  int width = 0;
  int height = 0;
  double fps = 0.0;
  double durationSeconds = 0.0;
  std::string codecName = "Unknown";
};

class VideoReader {
 public:
  VideoReader(const std::string& video_path);
  ~VideoReader();

  bool openFile();
  void closeFile();

  // ဗီဒီယိုထဲက နောက် Frame တစ်ခုကို ဆက်တိုက်ဖတ်ပေးမယ့် function
  bool readNextFrame(std::vector<uint8_t>& out_pixels);

  // Thumbnail ယူမယ့် function (သူက သီးသန့် Decoder သုံးမှာမို့ playback ကို မနှောင့်ယှက်တော့ပါဘူး)
  std::vector<uint8_t> getVideoThumbnail(double seconds, int targetWidth = 0,
                                         int targetHeight = 0);

  VideoFormatInfo getFormatInfo() const { return formatInfo; }

 private:
  std::string inputPath;
  VideoFormatInfo formatInfo;

  // FFmpeg Contexts (Video အတွက် သီးသန့်)
  AVFormatContext* formatContext = nullptr;
  AVCodecContext* videoCodecContext = nullptr;
  int videoStreamIndex = -1;

  // Reusable structures
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  AVFrame* rgbFrame = nullptr;
  struct SwsContext* swsCtx = nullptr;

  void cleanup();
};