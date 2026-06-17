#include "audio_file_saver.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <vector>

bool AudioFileSaver::saveAsWav(AudioDecoder& decoder,
                               const std::string& outPath) {
  if (!decoder.openFile()) return false;

  std::ofstream outFile(outPath, std::ios::binary);
  if (!outFile.is_open()) return false;

  AudioFormatInfo fmt = decoder.getTargetFormat();

  // ၁။ ၄၄ ဘိုက်စာ WAV Header နေရာကို ချန်ထားပြီး အရင်ကျော်ရေးမယ်
  // ဘာလို့လဲဆိုတော့ တစ်ဖိုင်လုံးဖတ်ပြီးမှ Audio data size ဘယ်လောက်ရှိလဲ သိမှာမို့လို့ပါ
  char blankHeader[44] = {0};
  outFile.write(blankHeader, 44);

  std::vector<uint8_t> chunk;
  int totalDataSize = 0;

  // ၂။ Audio chunk တွေအကုန်လုံးကို ဆက်တိုက် ရေးသွားမယ်
  while (decoder.readNextAudioChunk(chunk)) {
    if (!chunk.empty()) {
      outFile.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
      totalDataSize += chunk.size();
    }
  }

  // ၃။ ဒေတာအားလုံး ရေးပြီးပြီဆိုမှ ဖိုင်ရဲ့ အစဆုံးကို ပြန်သွားပြီး Header အမှန်ကို ထည့်မယ်
  outFile.seekp(0, std::ios::beg);
  writeWavHeader(outFile, fmt.sampleRate, fmt.channels, totalDataSize);

  outFile.close();
  std::cout << "[AudioFileSaver] WAV saved successfully: " << outPath
            << std::endl;
  return true;
}

// ==========================================
// ၁။ WAV File Saver (FFmpeg Encoder မလို၊ Header ပဲထည့်ရုံ)
// ==========================================
void AudioFileSaver::writeWavHeader(std::ofstream& file, int sampleRate,
                                    int channels, int totalDataSize) {
  int bitsPerSample = 16;  // ကျွန်တော်တို့ targetFormat က S16 မို့လို့ 16-bit ပါ
  int byteRate = sampleRate * channels * bitsPerSample / 8;
  int blockAlign = channels * bitsPerSample / 8;

  file.write("RIFF", 4);
  int chunkSize = 36 + totalDataSize;
  file.write(reinterpret_cast<char*>(&chunkSize), 4);
  file.write("WAVE", 4);
  file.write("fmt ", 4);

  int subChunk1Size = 16;  // PCM format size
  file.write(reinterpret_cast<char*>(&subChunk1Size), 4);

  short audioFormat = 1;  // 1 = PCM Uncompressed
  file.write(reinterpret_cast<char*>(&audioFormat), 2);

  short numChannels = static_cast<short>(channels);
  file.write(reinterpret_cast<char*>(&numChannels), 2);

  file.write(reinterpret_cast<char*>(&sampleRate), 4);
  file.write(reinterpret_cast<char*>(&byteRate), 4);

  short align = static_cast<short>(blockAlign);
  file.write(reinterpret_cast<char*>(&align), 2);

  short bps = static_cast<short>(bitsPerSample);
  file.write(reinterpret_cast<char*>(&bps), 2);

  file.write("data", 4);
  file.write(reinterpret_cast<char*>(&totalDataSize), 4);
}

// ADTS Header (7 bytes) ရေးပေးမယ့် Helper Function
void writeAdtsHeader(uint8_t* packet, int dataLength, int sampleRate,
                     int channels) {
  // Sampling frequencies mapping table
  int samplingFreqIndex = 4;  // Default 44100 Hz
  int rates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                 22050, 16000, 12000, 11025, 8000,  7350};
  for (int i = 0; i < 13; i++) {
    if (sampleRate == rates[i]) {
      samplingFreqIndex = i;
      break;
    }
  }

  int totalLength = dataLength + 7;
  packet[0] = 0xFF;  // Syncword
  packet[1] = 0xF1;  // Syncword (MPEG-4, No CRC)
  packet[2] =
      ((1 << 6) /* AAC LC */ | (samplingFreqIndex << 2) | (channels >> 2)) &
      0xFF;
  packet[3] = ((channels & 3) << 6) | (totalLength >> 11);
  packet[4] = (totalLength >> 3) & 0xFF;
  packet[5] = ((totalLength & 7) << 5) | 0x1F;
  packet[6] = 0xFC;
}

bool AudioFileSaver::saveAsAac(AudioDecoder& decoder,
                               const std::string& outPath) {
  if (!decoder.openFile()) return false;

  AudioFormatInfo inputFmt = decoder.getTargetFormat();

  // ၁။ AAC Encoder ရှာဖွေခြင်း
  const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (!encoder) {
    std::cerr << "[AudioFileSaver] AAC Encoder not found!" << std::endl;
    return false;
  }

  AVCodecContext* encCtx = avcodec_alloc_context3(encoder);
  encCtx->bit_rate = 128000;
  encCtx->sample_rate = inputFmt.sampleRate;
  encCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;  // AAC အတွက် Float Planar
  av_channel_layout_default(&encCtx->ch_layout, inputFmt.channels);

  if (avcodec_open2(encCtx, encoder, nullptr) < 0) {
    std::cerr << "[AudioFileSaver] Could not open AAC encoder." << std::endl;
    avcodec_free_context(&encCtx);
    return false;
  }

  // ၂။ Resampler Setup (S16 -> FLTP)
  SwrContext* aacSwr = nullptr;
  swr_alloc_set_opts2(&aacSwr, &encCtx->ch_layout, encCtx->sample_fmt,
                      encCtx->sample_rate, &encCtx->ch_layout,
                      inputFmt.sampleFmt, inputFmt.sampleRate, 0, nullptr);
  if (!aacSwr || swr_init(aacSwr) < 0) {
    avcodec_free_context(&encCtx);
    return false;
  }

  // ၃။ FIFO Queue ဆောက်ခြင်း (၁၀၂၄ ပြည့်မှ Encoder ဆီ ပို့ရန်)
  AVAudioFifo* fifo =
      av_audio_fifo_alloc(encCtx->sample_fmt, encCtx->ch_layout.nb_channels, 1);

  std::ofstream outFile(outPath, std::ios::binary);
  std::vector<uint8_t> chunk;
  AVFrame* encFrame = av_frame_alloc();
  AVPacket* encPacket = av_packet_alloc();

  // ၄။ Decoder ထံမှ ဒေတာများ ဖတ်ယူခြင်း
  while (decoder.readNextAudioChunk(chunk)) {
    if (chunk.empty()) continue;

    int bytesPerSample = av_get_bytes_per_sample(inputFmt.sampleFmt);
    int numSamples = chunk.size() / (inputFmt.channels * bytesPerSample);

    // ယာယီ ခွဲထုတ်မည့် Frame ဆောက်ခြင်း
    AVFrame* resampledFrame = av_frame_alloc();
    resampledFrame->nb_samples = numSamples;
    resampledFrame->format = encCtx->sample_fmt;
    av_channel_layout_copy(&resampledFrame->ch_layout, &encCtx->ch_layout);
    av_frame_get_buffer(resampledFrame, 0);

    const uint8_t* srcData = chunk.data();
    swr_convert(aacSwr, resampledFrame->data, numSamples, &srcData, numSamples);

    // ပြောင်းလဲပြီးသား အသံတွေကို FIFO Queue ထဲ ထည့်ထားမယ်
    av_audio_fifo_write(fifo, (void**)resampledFrame->data, numSamples);
    av_frame_free(&resampledFrame);

    // Queue ထဲမှာ AAC ရဲ့ စံနှုန်းဖြစ်တဲ့ frame_size (ဥပမာ ၁၀၂၄) ပြည့်တဲ့အထိ Loop ပတ်ပြီး ထုတ်ယူ
    // encode လုပ်မယ်
    while (av_audio_fifo_size(fifo) >= encCtx->frame_size) {
      encFrame->nb_samples = encCtx->frame_size;
      encFrame->format = encCtx->sample_fmt;
      av_channel_layout_copy(&encFrame->ch_layout, &encCtx->ch_layout);
      av_frame_get_buffer(encFrame, 0);

      av_audio_fifo_read(fifo, (void**)encFrame->data, encCtx->frame_size);

      if (avcodec_send_frame(encCtx, encFrame) >= 0) {
        while (avcodec_receive_packet(encCtx, encPacket) >= 0) {
          // ADTS Header (၇ ဘိုက်) ရေးပြီးမှ AAC data ကို လိုက်ရေးရမယ်
          uint8_t adts[7];
          writeAdtsHeader(adts, encPacket->size, encCtx->sample_rate,
                          encCtx->ch_layout.nb_channels);
          outFile.write(reinterpret_cast<char*>(adts), 7);

          outFile.write(reinterpret_cast<char*>(encPacket->data),
                        encPacket->size);
          av_packet_unref(encPacket);
        }
      }
      av_frame_unref(encFrame);
    }
  }

  // ၅။ Flush FIFO & Encoder (အမြီးကျန် နှာမောင်းကျန်များကို အကုန်ထုတ်ယူခြင်း)
  if (av_audio_fifo_size(fifo) > 0) {
    int remainingSamples = av_audio_fifo_size(fifo);
    encFrame->nb_samples = remainingSamples;
    encFrame->format = encCtx->sample_fmt;
    av_channel_layout_copy(&encFrame->ch_layout, &encCtx->ch_layout);
    av_frame_get_buffer(encFrame, 0);
    av_audio_fifo_read(fifo, (void**)encFrame->data, remainingSamples);

    if (avcodec_send_frame(encCtx, encFrame) >= 0) {
      while (avcodec_receive_packet(encCtx, encPacket) >= 0) {
        uint8_t adts[7];
        writeAdtsHeader(adts, encPacket->size, encCtx->sample_rate,
                        encCtx->ch_layout.nb_channels);
        outFile.write(reinterpret_cast<char*>(adts), 7);
        outFile.write(reinterpret_cast<char*>(encPacket->data),
                      encPacket->size);
        av_packet_unref(encPacket);
      }
    }
    av_frame_unref(encFrame);
  }

  avcodec_send_frame(encCtx, nullptr);
  while (avcodec_receive_packet(encCtx, encPacket) >= 0) {
    uint8_t adts[7];
    writeAdtsHeader(adts, encPacket->size, encCtx->sample_rate,
                    encCtx->ch_layout.nb_channels);
    outFile.write(reinterpret_cast<char*>(adts), 7);
    outFile.write(reinterpret_cast<char*>(encPacket->data), encPacket->size);
    av_packet_unref(encPacket);
  }

  // Clean up
  av_frame_free(&encFrame);
  av_packet_free(&encPacket);
  av_audio_fifo_free(fifo);
  swr_free(&aacSwr);
  avcodec_free_context(&encCtx);
  outFile.close();

  std::cout << "[AudioFileSaver] AAC saved successfully with correct size: "
            << outPath << std::endl;
  return true;
}
// ==========================================
// ၂။ MP3 File Saver (FFmpeg MP3 Encoder သုံးပြီး ချုံ့မယ်)
// ==========================================
bool AudioFileSaver::saveAsMp3(AudioDecoder& decoder,
                               const std::string& outPath) {
  if (!decoder.openFile()) return false;

  AudioFormatInfo inputFmt = decoder.getTargetFormat();

  // ၁။ MP3 Encoder (libmp3lame) ရှာပြီး Context ပြင်ဆင်မယ်
  const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
  if (!encoder) {
    std::cerr
        << "[AudioFileSaver] MP3 Encoder not found! (Check your FFmpeg build)"
        << std::endl;
    return false;
  }

  AVCodecContext* encCtx = avcodec_alloc_context3(encoder);
  encCtx->bit_rate = 128000;  // 128 kbps (Standard MP3)
  encCtx->sample_rate = inputFmt.sampleRate;
  encCtx->sample_fmt =
      AV_SAMPLE_FMT_S16P;  // MP3 Encoder အများစုက Planar format တောင်းတတ်တယ်
  av_channel_layout_default(&encCtx->ch_layout, inputFmt.channels);

  if (avcodec_open2(encCtx, encoder, nullptr) < 0) {
    std::cerr << "[AudioFileSaver] Could not open MP3 encoder." << std::endl;
    avcodec_free_context(&encCtx);
    return false;
  }

  // ၂။ MP3 encoding အတွက် ဒေတာတွေကို Format ပြောင်းဖို့ Resampler ထပ်လိုပါတယ် (S16 -> S16P
  // ပြောင်းရန်)
  SwrContext* mp3Swr = nullptr;
  swr_alloc_set_opts2(&mp3Swr, &encCtx->ch_layout, encCtx->sample_fmt,
                      encCtx->sample_rate, &encCtx->ch_layout,
                      inputFmt.sampleFmt, inputFmt.sampleRate, 0, nullptr);
  if (!mp3Swr || swr_init(mp3Swr) < 0) {
    std::cerr << "[AudioFileSaver] Failed to init MP3 Resampler." << std::endl;
    avcodec_free_context(&encCtx);
    return false;
  }

  std::ofstream outFile(outPath, std::ios::binary);
  std::vector<uint8_t> chunk;
  AVFrame* encFrame = av_frame_alloc();
  AVPacket* encPacket = av_packet_alloc();

  // ၃။ Decoder က ထွက်လာတဲ့ PCM data တွေကို ဖတ်ပြီး MP3 ပြောင်းမယ်
  while (decoder.readNextAudioChunk(chunk)) {
    if (chunk.empty()) continue;

    // လက်ရှိ chunk ရဲ့ sample အရေအတွက်
    int bytesPerSample = av_get_bytes_per_sample(inputFmt.sampleFmt);
    int numSamples = chunk.size() / (inputFmt.channels * bytesPerSample);

    // Encoder frame သတ်မှတ်ချက်
    encFrame->nb_samples = numSamples;
    encFrame->format = encCtx->sample_fmt;
    av_channel_layout_copy(&encFrame->ch_layout, &encCtx->ch_layout);
    av_frame_get_buffer(encFrame, 0);

    // S16 (Interleaved) မှ S16P (Planar) သို့ ပြောင်းလဲခြင်း
    const uint8_t* srcData = chunk.data();
    swr_convert(mp3Swr, encFrame->data, numSamples, &srcData, numSamples);

    // Encode လုပ်ပြီး MP3 Packet အဖြစ် ဖိုင်ထဲသိမ်းခြင်း
    if (avcodec_send_frame(encCtx, encFrame) >= 0) {
      while (avcodec_receive_packet(encCtx, encPacket) >= 0) {
        outFile.write(reinterpret_cast<char*>(encPacket->data),
                      encPacket->size);
        av_packet_unref(encPacket);
      }
    }
    av_frame_unref(encFrame);
  }

  // Flush Encoder (ကျန်နေတာတွေ အကုန်ထုတ်မယ်)
  avcodec_send_frame(encCtx, nullptr);
  while (avcodec_receive_packet(encCtx, encPacket) >= 0) {
    outFile.write(reinterpret_cast<char*>(encPacket->data), encPacket->size);
    av_packet_unref(encPacket);
  }

  // Clean up
  av_frame_free(&encFrame);
  av_packet_free(&encPacket);
  swr_free(&mp3Swr);
  avcodec_free_context(&encCtx);
  outFile.close();

  std::cout << "[AudioFileSaver] MP3 saved successfully: " << outPath
            << std::endl;
  return true;
}