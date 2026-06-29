#include "media_file.hpp"

#include <fstream>
#include <iostream>

#include "stb_image_write.h"

MediaFile::MediaFile(const std::string& video_path,
                     const AudioFormatInfo& target_format)
    : inputPath(video_path), targetFormat(target_format) {}

MediaFile::~MediaFile() { closeFile(); }

bool MediaFile::openFile() {
  // 1. Open Input File
  if (avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr) <
      0) {
    std::cerr << "[MediaFile] Error: Could not open file: " << inputPath
              << std::endl;
    return false;
  }

  // 2. Find Stream Info
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    std::cerr << "[MediaFile] Error: Could not find stream info." << std::endl;
    return false;
  }

  // 3. Find Audio Stream
  const AVCodec* decoder = nullptr;
  audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1,
                                         -1, &decoder, 0);
  if (audioStreamIndex < 0) {
    std::cerr << "[MediaFile] Error: No audio stream found." << std::endl;
    return false;
  }

  // 4. Allocate Codec Context
  codecContext = avcodec_alloc_context3(decoder);
  if (!codecContext) {
    std::cerr << "[MediaFile] Error: Failed to allocate codec context."
              << std::endl;
    return false;
  }

  // 5. Copy parameters to context
  if (avcodec_parameters_to_context(
          codecContext, formatContext->streams[audioStreamIndex]->codecpar) <
      0) {
    std::cerr << "[MediaFile] Error: Failed to copy parameters to context."
              << std::endl;
    return false;
  }

  // 6. Open Decoder Codec
  if (avcodec_open2(codecContext, decoder, nullptr) < 0) {
    std::cerr << "[MediaFile] Error: Failed to open codec." << std::endl;
    return false;
  }

  // 7. Initialize Resampler
  if (!initResampler()) {
    return false;
  }

  // 8. Packets နဲ့ Frames တွေကို တစ်ခါတည်း နေရာချထားမယ် (Reusable ဖြစ်အောင်)
  packet = av_packet_alloc();
  frame = av_frame_alloc();
  return true;
}

bool MediaFile::initResampler() {
  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, targetFormat.channels);

  // FFmpeg 8.x modern resampling structure
  int ret = swr_alloc_set_opts2(
      &swrCtx, &out_ch_layout, targetFormat.sampleFmt, targetFormat.sampleRate,
      &codecContext->ch_layout, codecContext->sample_fmt,
      codecContext->sample_rate, 0, nullptr);

  av_channel_layout_uninit(&out_ch_layout);  // Clean up temp layout immediately

  if (ret < 0 || !swrCtx || swr_init(swrCtx) < 0) {
    std::cerr << "[MediaFile] Error: Resampler initialization failed."
              << std::endl;
    return false;
  }
  return true;
}

bool MediaFile::readNextAudioChunk(std::vector<uint8_t>& out_chunk) {
  out_chunk.clear();  // ယခင် Chunk အဟောင်းကို ရှင်းထုတ်မယ်

  int ret;
  // ဗီဒီယိုဖိုင်ထဲကနေ packet ဖတ်မယ်
  while (av_read_frame(formatContext, packet) >= 0) {
    // အကယ်၍ အသံ stream ဖြစ်ခဲ့ရင်
    if (packet->stream_index == audioStreamIndex) {
      ret = avcodec_send_packet(codecContext, packet);
      av_packet_unref(packet);  // Packet Memory ကို ရှင်းမယ်

      if (ret < 0) return false;

      // Packet တစ်ခုထဲကနေ Frame တွေအကုန်ထွက်တဲ့အထိ Loop ပတ်ပြီးဖတ်မယ်
      while (avcodec_receive_frame(codecContext, frame) >= 0) {
        // 🌟 [ပြင်ဆင်ချက် ၁ - ဒီနေရာမှာ ထည့်ရပါမယ်] 🌟
        // သီချင်းပုံမှန်ပတ်နေစဉ် Frame ရတိုင်း လက်ရှိ စက္ကန့်ကို အမြဲတမ်း Update လုပ်ပေးမယ်
        if (frame->pts != AV_NOPTS_VALUE) {
          currentTimestampSeconds =
              frame->pts *
              av_q2d(formatContext->streams[audioStreamIndex]->time_base);
        } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
          currentTimestampSeconds =
              frame->pkt_dts *
              av_q2d(formatContext->streams[audioStreamIndex]->time_base);
        }

        // ပြောင်းလဲပေးရမယ့် Sample ပမာဏကို တွက်ချက်ခြင်း
        int max_out_samples = swr_get_delay(swrCtx, codecContext->sample_rate) +
                              frame->nb_samples;
        uint8_t* output_buffer = nullptr;

        ret = av_samples_alloc(&output_buffer, nullptr, targetFormat.channels,
                               max_out_samples, targetFormat.sampleFmt, 0);
        if (ret < 0) {
          av_frame_unref(frame);
          return false;
        }

        // တကယ့် Audio format ပြောင်းလဲခြင်း (Resampling)
        int out_samples =
            swr_convert(swrCtx, &output_buffer, max_out_samples,
                        (const uint8_t**)frame->data, frame->nb_samples);

        if (out_samples > 0) {
          int buffer_size = av_samples_get_buffer_size(
              nullptr, targetFormat.channels, out_samples,
              targetFormat.sampleFmt, 1);
          if (buffer_size > 0) {
            // ထွက်လာတဲ့ RAW PCM ဒေတာတွေကို out_chunk ထဲ တန်းထည့်ပေးလိုက်မယ်
            out_chunk.insert(out_chunk.end(), output_buffer,
                             output_buffer + buffer_size);
          }
        }

        av_freep(&output_buffer);
        av_frame_unref(frame);  // Frame Memory ကို ရှင်းမယ်

        // Chunk ထဲကို ဒေတာရောက်သွားပြီဆိုရင် ဒီတစ်ခေါက်အတွက် Loop ကနေ ထွက်ပြီး အပြင် Class ကို
        // လှမ်းပေးမယ်
        if (!out_chunk.empty()) {
          return true;
        }
      }
    } else {
      // Audio မဟုတ်ရင် ကျော်သွားမယ်
      av_packet_unref(packet);
    }
  }

  // ဖိုင်အဆုံးသတ် (EOF) ရောက်သွားရင် ကျန်နေသေးတဲ့ ဒေတာတွေ (Flush) ထုတ်ပေးဖို့
  ret = avcodec_send_packet(codecContext, nullptr);
  if (ret >= 0 && avcodec_receive_frame(codecContext, frame) >= 0) {
    // 🌟 [ပြင်ဆင်ချက် ၂ - Flush တဲ့နေရာမှာလည်း ထားခဲ့ပေးပါမယ်] 🌟
    if (frame->pts != AV_NOPTS_VALUE) {
      currentTimestampSeconds =
          frame->pts *
          av_q2d(formatContext->streams[audioStreamIndex]->time_base);
    } else if (frame->pkt_dts != AV_NOPTS_VALUE) {
      currentTimestampSeconds =
          frame->pkt_dts *
          av_q2d(formatContext->streams[audioStreamIndex]->time_base);
    }

    int max_out_samples =
        swr_get_delay(swrCtx, codecContext->sample_rate) + frame->nb_samples;
    uint8_t* output_buffer = nullptr;
    av_samples_alloc(&output_buffer, nullptr, targetFormat.channels,
                     max_out_samples, targetFormat.sampleFmt, 0);

    int out_samples =
        swr_convert(swrCtx, &output_buffer, max_out_samples,
                    (const uint8_t**)frame->data, frame->nb_samples);
    if (out_samples > 0) {
      int buffer_size =
          av_samples_get_buffer_size(nullptr, targetFormat.channels,
                                     out_samples, targetFormat.sampleFmt, 1);
      out_chunk.insert(out_chunk.end(), output_buffer,
                       output_buffer + buffer_size);
    }
    av_freep(&output_buffer);
    av_frame_unref(frame);
    return !out_chunk.empty();
  }

  return false;
}

bool MediaFile::seekToSeconds(double seconds) {
  // စက္ကန့်ကို FFmpeg Timebase အဖြစ် ပြောင်းလဲခြင်း
  int64_t targetTimestamp =
      seconds / av_q2d(formatContext->streams[audioStreamIndex]->time_base);

  // FFmpeg seek function ကို ခေါ်ခြင်း
  if (av_seek_frame(formatContext, audioStreamIndex, targetTimestamp,
                    AVSEEK_FLAG_BACKWARD) < 0) {
    return false;  // Seek မအောင်မြင်ပါ
  }

  // Seek လုပ်ပြီးရင် codec ရဲ့ buffer တွေကို clear လုပ်ပေးရပါမယ်
  avcodec_flush_buffers(codecContext);
  return true;
}

double MediaFile::getDurationInSeconds() {
  if (!formatContext) return 0.0;

  // AV_TIME_BASE_Q က Microseconds ကို Seconds ဖြစ်အောင် ပြောင်းပေးပါတယ်
  // (duration ကို double နဲ့စားပြီး စက္ကန့်အဖြစ် ပြောင်းလဲခြင်း)
  return formatContext->duration / (double)AV_TIME_BASE;
}

double MediaFile::getCurrentInSeconds() { return currentTimestampSeconds; }

std::string MediaFile::getMetadata(const std::string& key) {
  if (!formatContext || !formatContext->metadata) {
    return "";
  }

  // av_dict_get က key ကို လိုက်ရှာပေးပါတယ် (AV_DICT_MATCH_CASE က စာလုံးအကြီးအသေး
  // ခွဲခြားတာပါ)
  AVDictionaryEntry* tag = av_dict_get(formatContext->metadata, key.c_str(),
                                       nullptr, AV_DICT_MATCH_CASE);

  if (tag && tag->value) {
    return std::string(tag->value);
  }

  return "";
}

// audio_decoder.cpp ထဲတွင် Album Art ထုတ်ရန်
std::vector<uint8_t> MediaFile::getAlbumArt() {
  std::vector<uint8_t> artBuffer;

  if (!formatContext) {
    return artBuffer;
  }

  // ၁။ သီချင်းဖိုင်ထဲမှာပါတဲ့ stream အားလုံးကို လိုက်စစ်မယ်
  for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
    // ၂။ ဒီ stream က သီချင်းထဲမှာ ကပ်ပါလာတဲ့ ပုံ (Attached Picture) ဟုတ်မဟုတ် စစ်တယ်
    if (formatContext->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
      // ၃။ ပုံရဲ့ packet ဒေတာကို ဆွဲထုတ်တယ်
      AVPacket pkt = formatContext->streams[i]->attached_pic;

      // ၄။ ပုံဒေတာ တကယ်ရှိရင် artBuffer ထဲကို bytes အကုန် ကူးထည့်လိုက်မယ်
      if (pkt.size > 0 && pkt.data) {
        artBuffer.insert(artBuffer.end(), pkt.data, pkt.data + pkt.size);
        return artBuffer;  // ပုံရပြီဖြစ်လို့ ကျန်တဲ့ stream တွေ ဆက်မပတ်တော့ဘဲ တန်းပြန်မယ်
      }
    }
  }

  // ပုံမပါတဲ့ သီချင်းဆိုရင်တော့ buffer အလွတ်ပဲ ပြန်လိမ့်မယ်
  return artBuffer;
}

void MediaFile::closeFile() {
  if (frame) av_frame_free(&frame);
  if (packet) av_packet_free(&packet);
  if (codecContext) avcodec_free_context(&codecContext);
  if (formatContext) avformat_close_input(&formatContext);
  if (swrCtx) swr_free(&swrCtx);
}

//***************Video********************* */
// Memory Buffer ထဲသို့ FFmpeg မှ JPEG data တိုက်ရိုက်ရေးရန် အကူအညီ function

std::vector<uint8_t> MediaFile::getVideoThumbnail(double seconds,
                                                  int targetWidth,
                                                  int targetHeight) {
  std::vector<uint8_t> jpegBuffer;

  if (!formatContext) return jpegBuffer;

  if (videoStreamIndex == -1) {
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
      if (formatContext->streams[i]->codecpar->codec_type ==
          AVMEDIA_TYPE_VIDEO) {
        videoStreamIndex = i;
        break;
      }
    }
  }
  if (videoStreamIndex == -1) return jpegBuffer;

  AVStream* videoStream = formatContext->streams[videoStreamIndex];
  const AVCodec* videoDecoder =
      avcodec_find_decoder(videoStream->codecpar->codec_id);
  if (!videoDecoder) return jpegBuffer;

  AVCodecContext* decCtx = avcodec_alloc_context3(videoDecoder);
  avcodec_parameters_to_context(decCtx, videoStream->codecpar);

  decCtx->thread_count = 0;

  if (avcodec_open2(decCtx, videoDecoder, nullptr) < 0) {
    avcodec_free_context(&decCtx);
    return jpegBuffer;
  }

  // ၁။ Seek လုပ်ခြင်း (FFmpeg 8.x အတွက် ပိုမိုစိတ်ချရသော flag)
  int64_t targetTimestamp =
      static_cast<int64_t>(seconds / av_q2d(videoStream->time_base));
  av_seek_frame(formatContext, videoStreamIndex, targetTimestamp,
                AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
  avcodec_flush_buffers(decCtx);

  AVFrame* videoFrame = av_frame_alloc();
  AVPacket* vidPacket = av_packet_alloc();
  bool frameDecoded = false;

  // ၂။ FFmpeg 8.x ရဲ့ packet reading logic ကို ပိုမိုကျစ်လျစ်အောင် ပြင်ဆင်ခြင်း
  int readAttempts = 0;
  while (av_read_frame(formatContext, vidPacket) >= 0 && readAttempts < 100) {
    if (vidPacket->stream_index == videoStreamIndex) {
      int sendRes = avcodec_send_packet(decCtx, vidPacket);
      if (sendRes == 0) {
        int recRes = avcodec_receive_frame(decCtx, videoFrame);
        if (recRes == 0) {
          frameDecoded = true;
          av_packet_unref(vidPacket);
          break;
        } else if (recRes == AVERROR(EAGAIN)) {
          // Frame မပြည့်စုံသေးလို့ နောက် packet တစ်ခု ထပ်ဖတ်ဖို့ လိုအပ်တာပါ
          av_packet_unref(vidPacket);
          continue;
        }
      }
    }
    av_packet_unref(vidPacket);
    readAttempts++;
  }

  // အကယ်၍ loop ထဲမှာ keyframe မမိခဲ့ရင် decoder ထဲမှာ ကျန်နေတာကို ဇွတ်ဆွဲထုတ်မယ် (Flush)
  if (!frameDecoded) {
    avcodec_send_packet(decCtx, nullptr);
    if (avcodec_receive_frame(decCtx, videoFrame) == 0) {
      frameDecoded = true;
    }
  }

  // ၃။ Frame ရပြီဆိုရင် JPEG ပြောင်းလဲခြင်း
  if (frameDecoded) {
    int outWidth = (targetWidth <= 0) ? decCtx->width : targetWidth;
    int outHeight = (targetHeight <= 0) ? decCtx->height : targetHeight;

    const AVCodec* jpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (jpegCodec) {
      AVCodecContext* encCtx = avcodec_alloc_context3(jpegCodec);

      encCtx->width = outWidth;
      encCtx->height = outHeight;
      // FFmpeg 8.x အတွက် pixel format ကို မူရင်းဗီဒီယိုအတိုင်း fallback ဖြစ်အောင် ထိန်းညှိပေးရပါမယ်
      // အသစ်ပြင်ဆင်ချက်- deprecated ဖြစ်တဲ့ YUVJ420P အစား YUV420P ကို သုံးပါမယ်
      encCtx->pix_fmt = AV_PIX_FMT_YUV420P;

      // ကွန်ပိုင်လာ အော်နေတဲ့ range ကို ဒီမှာ သေချာ သတ်မှတ်ပေးလိုက်တာပါ
      encCtx->color_range = AVCOL_RANGE_JPEG;

      encCtx->time_base = {1, 25};

      if (avcodec_open2(encCtx, jpegCodec, nullptr) >= 0) {
        AVFrame* jpegFrame = av_frame_alloc();
        jpegFrame->width = outWidth;
        jpegFrame->height = outHeight;
        jpegFrame->format = encCtx->pix_fmt;

        if (av_frame_get_buffer(jpegFrame, 0) >= 0) {
          struct SwsContext* swsCtx = sws_getContext(
              videoFrame->width, videoFrame->height,
              (AVPixelFormat)videoFrame->format, outWidth, outHeight,
              encCtx->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);

          if (swsCtx) {
            sws_scale(swsCtx, videoFrame->data, videoFrame->linesize, 0,
                      videoFrame->height, jpegFrame->data, jpegFrame->linesize);
            sws_freeContext(swsCtx);

            if (avcodec_send_frame(encCtx, jpegFrame) == 0) {
              AVPacket* encPacket = av_packet_alloc();
              if (avcodec_receive_packet(encCtx, encPacket) == 0) {
                jpegBuffer.assign(encPacket->data,
                                  encPacket->data + encPacket->size);
              }
              av_packet_free(&encPacket);
            }
          }
        }
        av_frame_unref(jpegFrame);
        av_frame_free(&jpegFrame);
      }
      avcodec_free_context(&encCtx);
    }
  }

  // Cleanup
  av_frame_unref(videoFrame);
  av_frame_free(&videoFrame);
  av_packet_free(&vidPacket);
  avcodec_free_context(&decCtx);

  return jpegBuffer;
}
bool MediaFile::saveAsVideoThumbnail(const std::string& outPath, double seconds,
                                     int targetWidth, int targetHeight) {
  auto thumbnail = getVideoThumbnail(seconds, targetWidth, targetHeight);
  // std::cout << "size: "<<thumbnail.size() << "\n";
  if (thumbnail.empty()) return false;

  std::ofstream outFile(outPath, std::ios::out | std::ios::binary);

  if (!outFile) return false;
  outFile.write(reinterpret_cast<const char*>(thumbnail.data()),
                thumbnail.size());
  outFile.close();

  return true;
}
