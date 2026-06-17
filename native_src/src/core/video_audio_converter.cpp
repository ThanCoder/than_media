#include "video_audio_converter.hpp"

#include <cstdint>
#include <iostream>

struct WAVHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  int32_t overall_size = 0;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt_chunk_marker[4] = {'f', 'm', 't', ' '};
  int32_t length_of_fmt = 16;
  int16_t format_type = 1;      // PCM
  int16_t channels = 2;         // Target: Stereo
  int32_t sample_rate = 44100;  // Target: 44100Hz
  int32_t byterate = 44100 * 2 * 2;
  int16_t block_align = 2 * 2;
  int16_t bits_per_sample = 16;  // Target: 16-bit
  char data_chunk_header[4] = {'d', 'a', 't', 'a'};
  int32_t data_size = 0;
};

VideoAudioConverter::VideoAudioConverter(const std::string& video_path,
                                         const std::string& out_path)
    : inputPath(video_path), outputPath(out_path) {}

VideoAudioConverter::~VideoAudioConverter() {}

bool VideoAudioConverter::initialize() {
  if (avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr) <
      0)
    return false;
  if (avformat_find_stream_info(formatContext, nullptr) < 0) return false;

  const AVCodec* decoder = nullptr;
  audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1,
                                         -1, &decoder, 0);
  if (audioStreamIndex < 0) return false;

  codecContext = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(
      codecContext, formatContext->streams[audioStreamIndex]->codecpar);
  if (avcodec_open2(codecContext, decoder, nullptr) < 0) return false;

  // --- Modern FFmpeg 8.x Resampler Setup ---
  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, TARGET_CHANNELS);  // Stereo Layout

  int ret =
      swr_alloc_set_opts2(&swrCtx, &out_ch_layout, TARGET_SAMPLE_FMT,
                          TARGET_SAMPLE_RATE,  // Output configuration
                          &codecContext->ch_layout, codecContext->sample_fmt,
                          codecContext->sample_rate,  // Input configuration
                          0, nullptr);

  av_channel_layout_uninit(&out_ch_layout);  // memory clear

  if (ret < 0 || swr_init(swrCtx) < 0) {
    std::cerr << "Failed to initialize the resampling context" << std::endl;
    return false;
  }

  outputFile.open(outputPath, std::ios::binary);
  return outputFile.is_open();
}

int VideoAudioConverter::decodePacket(AVPacket* packet, AVFrame* frame) {
  int response = avcodec_send_packet(codecContext, packet);
  if (response < 0) return response;

  while (response >= 0) {
    response = avcodec_receive_frame(codecContext, frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
      break;
    else if (response < 0)
      return response;

    // ပြောင်းလဲပြီးသား အသံဒေတာ သိမ်းမည့် Output Buffer Pointer ဆောက်ခြင်း
    uint8_t* outputBuffer = nullptr;

    // Output ရမယ့် sample အရေအတွက်ကို တွက်ချက်ခြင်း
    int outSamples = swr_get_out_samples(swrCtx, frame->nb_samples);

    // Output buffer အတွက် memory နေရာချပေးခြင်း
    av_samples_alloc(&outputBuffer, nullptr, TARGET_CHANNELS, outSamples,
                     TARGET_SAMPLE_FMT, 0);

    // တကယ့် Audio Format အား 16-bit Signed PCM သို့ ပြောင်းလဲခြင်း (Resample)
    int convertedSamples =
        swr_convert(swrCtx, &outputBuffer, outSamples,
                    (const uint8_t**)frame->data, frame->nb_samples);

    if (convertedSamples > 0) {
      // Buffer size တွက်ပြီး ဖိုင်ထဲသို့ ရေးထည့်ခြင်း
      int linesize = convertedSamples * TARGET_CHANNELS *
                     av_get_bytes_per_sample(TARGET_SAMPLE_FMT);
      outputFile.write(reinterpret_cast<char*>(outputBuffer), linesize);
    }

    av_freep(&outputBuffer);  // သုံးပြီးသား ခဏတာ buffer ကို ဖျက်ခြင်း
  }
  return 0;
}

bool VideoAudioConverter::convert() {
  if (!initialize()) return false;
  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();

  WAVHeader
      header;  // Default အတိုင်း အပေါ်ဆုံးက Target 16-bit, 44100Hz, Stereo ဝင်သွားမည်
  outputFile.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

  std::cout << "Converting audio with resampling..." << std::endl;

  while (av_read_frame(formatContext, packet) >= 0) {
    if (packet->stream_index == audioStreamIndex) {
      decodePacket(packet, frame);
    }
    av_packet_unref(packet);
  }
  decodePacket(nullptr, frame);  // Flush

  // WAV Header ရဲ့ Size များကို ပြန်ပြင်ခြင်း
  int64_t total_data_size =
      static_cast<int64_t>(outputFile.tellp()) - sizeof(WAVHeader);
  header.overall_size = total_data_size + 36;
  header.data_size = total_data_size;

  outputFile.seekp(0, std::ios::beg);
  outputFile.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

  std::cout << "Conversion completed successfully!" << std::endl;

  av_frame_free(&frame);
  av_packet_free(&packet);
  return true;
}

void VideoAudioConverter::cleanUp() {
  if (codecContext) {
    avcodec_free_context(&codecContext);
    codecContext = nullptr;
  }
  if (formatContext) {
    avformat_close_input(&formatContext);
    formatContext = nullptr;
  }
  if (outputFile.is_open()) {
    outputFile.close();
  }
}