#include "audio_decoder.hpp"

#include <iostream>

AudioDecoder::AudioDecoder(const std::string& video_path,
                           const AudioFormatInfo& target_format)
    : inputPath(video_path), targetFormat(target_format) {}

AudioDecoder::~AudioDecoder() { closeFile(); }

bool AudioDecoder::openFile() {
  // 1. Open Input File
  if (avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr) <
      0) {
    std::cerr << "[AudioDecoder] Error: Could not open file: " << inputPath
              << std::endl;
    return false;
  }

  // 2. Find Stream Info
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    std::cerr << "[AudioDecoder] Error: Could not find stream info."
              << std::endl;
    return false;
  }

  // 3. Find Audio Stream
  const AVCodec* decoder = nullptr;
  audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1,
                                         -1, &decoder, 0);
  if (audioStreamIndex < 0) {
    std::cerr << "[AudioDecoder] Error: No audio stream found." << std::endl;
    return false;
  }

  // 4. Allocate Codec Context
  codecContext = avcodec_alloc_context3(decoder);
  if (!codecContext) {
    std::cerr << "[AudioDecoder] Error: Failed to allocate codec context."
              << std::endl;
    return false;
  }

  // 5. Copy parameters to context
  if (avcodec_parameters_to_context(
          codecContext, formatContext->streams[audioStreamIndex]->codecpar) <
      0) {
    std::cerr << "[AudioDecoder] Error: Failed to copy parameters to context."
              << std::endl;
    return false;
  }

  // 6. Open Decoder Codec
  if (avcodec_open2(codecContext, decoder, nullptr) < 0) {
    std::cerr << "[AudioDecoder] Error: Failed to open codec." << std::endl;
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

bool AudioDecoder::initResampler() {
  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, targetFormat.channels);

  // FFmpeg 8.x modern resampling structure
  int ret = swr_alloc_set_opts2(
      &swrCtx, &out_ch_layout, targetFormat.sampleFmt, targetFormat.sampleRate,
      &codecContext->ch_layout, codecContext->sample_fmt,
      codecContext->sample_rate, 0, nullptr);

  av_channel_layout_uninit(&out_ch_layout);  // Clean up temp layout immediately

  if (ret < 0 || !swrCtx || swr_init(swrCtx) < 0) {
    std::cerr << "[AudioDecoder] Error: Resampler initialization failed."
              << std::endl;
    return false;
  }
  return true;
}

bool AudioDecoder::readNextAudioChunk(std::vector<uint8_t>& out_chunk) {
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

bool AudioDecoder::seekToSeconds(double seconds) {
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

double AudioDecoder::getDurationInSeconds() {
  if (!formatContext) return 0.0;

  // AV_TIME_BASE_Q က Microseconds ကို Seconds ဖြစ်အောင် ပြောင်းပေးပါတယ်
  // (duration ကို double နဲ့စားပြီး စက္ကန့်အဖြစ် ပြောင်းလဲခြင်း)
  return formatContext->duration / (double)AV_TIME_BASE;
}

double AudioDecoder::getCurrentInSeconds() { return currentTimestampSeconds; }

void AudioDecoder::closeFile() {
  if (frame) av_frame_free(&frame);
  if (packet) av_packet_free(&packet);
  if (codecContext) avcodec_free_context(&codecContext);
  if (formatContext) avformat_close_input(&formatContext);
  if (swrCtx) swr_free(&swrCtx);
}
