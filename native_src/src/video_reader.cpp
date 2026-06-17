#include "video_reader.hpp"

#include <iostream>

VideoReader::VideoReader(const std::string& video_path)
    : inputPath(video_path) {
  packet = av_packet_alloc();
  frame = av_frame_alloc();
  rgbFrame = av_frame_alloc();
}

VideoReader::~VideoReader() {
  closeFile();
  av_packet_free(&packet);
  av_frame_free(&frame);
  av_frame_free(&rgbFrame);
}

bool VideoReader::openFile() {
  if (avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr) <
      0) {
    std::cerr << "[Error] Could not open video file: " << inputPath << "\n";
    return false;
  }

  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    std::cerr << "[Error] Could not find stream information.\n";
    return false;
  }

  // Video Stream ရှာဖွေခြင်း
  for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStreamIndex = i;
      break;
    }
  }

  if (videoStreamIndex == -1) {
    std::cerr << "[Error] No video stream found.\n";
    return false;
  }

  // Decoder ဖွင့်လှစ်ခြင်း
  AVStream* stream = formatContext->streams[videoStreamIndex];
  const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!decoder) {
    std::cerr << "[Error] Decoder not found.\n";
    return false;
  }

  videoCodecContext = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(videoCodecContext, stream->codecpar);
  videoCodecContext->thread_count = 0;  // Multithreading ဖွင့်ခြင်း

  if (avcodec_open2(videoCodecContext, decoder, nullptr) < 0) {
    std::cerr << "[Error] Could not open codec.\n";
    return false;
  }

  // Format Info များကို တွက်ချက်သိမ်းဆည်းခြင်း
  formatInfo.width = videoCodecContext->width;
  formatInfo.height = videoCodecContext->height;

  if (stream->avg_frame_rate.den > 0)
    formatInfo.fps = av_q2d(stream->avg_frame_rate);

  if (stream->duration != AV_NOPTS_VALUE) {
    formatInfo.durationSeconds = stream->duration * av_q2d(stream->time_base);
  } else {
    formatInfo.durationSeconds =
        formatContext->duration / static_cast<double>(AV_TIME_BASE);
  }

  const AVCodecDescriptor* desc =
      avcodec_descriptor_get(stream->codecpar->codec_id);
  if (desc) formatInfo.codecName = desc->name;

  return true;
}

bool VideoReader::readNextFrame(std::vector<uint8_t>& out_pixels) {
  if (!formatContext || !videoCodecContext) return false;

  bool frameDecoded = false;

  // Video Packet သီးသန့် ရောက်လာတဲ့အထိပဲ ဖတ်မယ် (Audio တွေပါလာရင် auto ကျော်သွားမယ်)
  while (av_read_frame(formatContext, packet) >= 0) {
    if (packet->stream_index == videoStreamIndex) {
      if (avcodec_send_packet(videoCodecContext, packet) == 0) {
        if (avcodec_receive_frame(videoCodecContext, frame) == 0) {
          frameDecoded = true;
          av_packet_unref(packet);
          break;
        }
      }
    }
    av_packet_unref(packet);
  }

  if (frameDecoded) {
    int w = videoCodecContext->width;
    int h = videoCodecContext->height;
    AVPixelFormat outFormat = AV_PIX_FMT_RGB24;

    int numBytes = av_image_get_buffer_size(outFormat, w, h, 1);
    out_pixels.resize(numBytes);

    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, out_pixels.data(),
                         outFormat, w, h, 1);

    // Sws Context ကို Reusable လုပ်ထားလို့ ပိုမြန်ပါတယ်
    swsCtx = sws_getCachedContext(swsCtx, w, h, videoCodecContext->pix_fmt, w,
                                  h, outFormat, SWS_BILINEAR, nullptr, nullptr,
                                  nullptr);

    if (swsCtx) {
      sws_scale(swsCtx, frame->data, frame->linesize, 0, h, rgbFrame->data,
                rgbFrame->linesize);
    }
  }

  return frameDecoded;
}

void VideoReader::closeFile() {
  if (swsCtx) {
    sws_freeContext(swsCtx);
    swsCtx = nullptr;
  }
  if (videoCodecContext) {
    avcodec_free_context(&videoCodecContext);
    videoCodecContext = nullptr;
  }
  if (formatContext) {
    avformat_close_input(&formatContext);
    formatContext = nullptr;
  }
  videoStreamIndex = -1;
}