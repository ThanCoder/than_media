#include "audio_device.hpp"

#include <iostream>

void AudioDevice::processAudio(void* pOutput, const void* pInput,
                               ma_uint32 frameCount) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(pOutput);
  size_t bytesNeeded =
      frameCount * device.playback.channels * 2;  // s16 = 2 bytes
  size_t bytesWritten = 0;

  while (bytesWritten < bytesNeeded) {
    size_t bytesAvailable = leftoverBuffer.size() - bufferReadPointer;

    if (bytesAvailable > 0) {
      size_t bytesToCopy = (bytesAvailable > (bytesNeeded - bytesWritten))
                               ? (bytesNeeded - bytesWritten)
                               : bytesAvailable;
      std::memcpy(dst + bytesWritten, leftoverBuffer.data() + bufferReadPointer,
                  bytesToCopy);

      bytesWritten += bytesToCopy;
      bufferReadPointer += bytesToCopy;
    } else {
      leftoverBuffer.clear();
      bufferReadPointer = 0;

      bool hasMoreData = decoder.readNextAudioChunk(leftoverBuffer);

      if (!hasMoreData || leftoverBuffer.empty()) {
        break;
      }
    }
  }

  if (bytesWritten < bytesNeeded) {
    std::memset(dst + bytesWritten, 0, bytesNeeded - bytesWritten);
  }

  (void)pInput;
}

void AudioDevice::staticDataCallback(ma_device* pDevice, void* pOutput,
                                     const void* pInput, ma_uint32 frameCount) {
  // pUserData ထဲကနေ AudioDevice object ရဲ့ pointer ကို ပြန်ထုတ်ယူသည်
  AudioDevice* pAudioDevice =
      reinterpret_cast<AudioDevice*>(pDevice->pUserData);
  if (pAudioDevice) {
    pAudioDevice->processAudio(pOutput, pInput, frameCount);
  }
}

bool AudioDevice::init() {
  if (!decoder.openFile()) {
    std::cerr << "Open Error: " << decoder.getCurrentPath() << "\n";
    return false;
  }

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = 2;
  deviceConfig.sampleRate = 44100;

  // အရေးကြီးဆုံးအပိုင်း: Static callback ကို ညွှန်းပြီး၊ current object (this) ကို User
  // Data အဖြစ် ထည့်ပေးလိုက်ခြင်း
  deviceConfig.dataCallback = AudioDevice::staticDataCallback;
  deviceConfig.pUserData = this;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    std::cerr << "Error: Audio Device ကို Initialise လုပ်လို့မရပါ!" << std::endl;
    return false;
  }

  isInitialized = true;
  return true;
}
bool AudioDevice::start() {
  if (!isInitialized) return false;
  return (ma_device_start(&device) == MA_SUCCESS);
}
void AudioDevice::stop() {
  if (isInitialized) {
    ma_device_stop(&device);
  }
}
double AudioDevice::getCurrentInSeconds() { return decoder.getCurrentInSeconds(); }
double AudioDevice::getDurationInSeconds() { return decoder.getDurationInSeconds(); }

void AudioDevice::uninit() {
  if (isInitialized) {
    ma_device_uninit(&device);
    isInitialized = false;
    std::cout << "Device ပိတ်သိမ်းပြီးပါပြီ။" << std::endl;
  }
}

void AudioDevice::seek(double seconds) {
  // ၁။ miniaudio device ကို ခဏ Stop ထားပါ (Callback ထဲမှာ ဒေတာတွေ ကမောက်ကမ မဖြစ်အောင်)
  bool wasPlaying = device.state.value == ma_device_state_started;
  if (wasPlaying) {
    ma_device_stop(&device);
  }

  // ၂။ သင့်ရဲ့ AudioDecoder ထဲမှာ seek လုပ်မယ့် function ကို လှမ်းခေါ်ပါ
  // (ဥပမာ- decoder.seekToSeconds(seconds); )
  decoder.seekToSeconds(seconds);

  // ၃။ အရေးကြီးဆုံးအပိုင်း: Buffer အဟောင်းတွေကို အကုန်ဖျက်ပြီး pointer ကို ၀ ပြန်ထားပါ
  leftoverBuffer.clear();
  bufferReadPointer = 0;

  // ၄။ အသံပြန်ဖွင့်ပါ
  if (wasPlaying) {
    ma_device_start(&device);
  }
}