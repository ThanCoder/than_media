
#pragma once
#include <cstring>

#include "media_file.hpp"
#include "miniaudio.h"

class AudioDevice {
 private:
  MediaFile decoder;
  ma_device device;
  ma_device_config deviceConfig;
  bool isInitialized;

  // Buffer ပိုင်းဆိုင်ရာ variables များကို class အောက်သို့ ရွှေ့လိုက်သည်
  std::vector<uint8_t> leftoverBuffer;
  size_t bufferReadPointer;

  // ၁။ တကယ့် လုပ်ငန်းလုပ်မည့် Private Member Function (မပြောင်းလဲပါ)
  void processAudio(void* pOutput, const void* pInput, ma_uint32 frameCount);

  // ၂။ miniaudio က လှမ်းခေါ်မည့် Static Callback (Bridge အဖြစ် သုံးသည်)
  static void staticDataCallback(ma_device* pDevice, void* pOutput,
                                 const void* pInput, ma_uint32 frameCount);

 public:
  AudioDevice(const std::string& filePath)
      : decoder(filePath), bufferReadPointer(0), isInitialized(false) {}

  ~AudioDevice() { uninit(); }

  // Device နှင့် Decoder ကို အလုပ်လုပ်ရန် ပြင်ဆင်ခြင်း
  bool init();

  // သီချင်းဖွင့်ရန်
  bool start();

  // ခေတ္တရပ်ရန် / ရပ်ရန်
  void stop();

  // Resource များ ပြန်လွှတ်ရန်
  void uninit();
  double getCurrentInSeconds();
  double getDurationInSeconds();
  void seek(double seconds);
  void setVolume(float volume);

  void fade() {}
};