#include "audio_device.hpp"
#include "ffi_wrapper.h"

void* audio_device_create(const char* file_path) {
  auto dv = new AudioDevice(file_path);
  return dv;
}

void audio_device_destroy(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    delete dv;
  }
}

bool audio_device_init(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->init();
  }
  return false;
}
bool audio_device_start(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->start();
  }
  return false;
}
void audio_device_stop(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    dv->stop();
  }
}
void audio_device_uninit(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->uninit();
  }
}
double audio_device_getCurrentInSeconds(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->getCurrentInSeconds();
  }
}
double audio_device_getDurationInSeconds(void* audio_device_ptr) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->getDurationInSeconds();
  }
}
void audio_device_seek(void* audio_device_ptr, double seconds) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->seek(seconds);
  }
}
void audio_device_setVolume(void* audio_device_ptr, float volume) {
  auto dv = reinterpret_cast<AudioDevice*>(audio_device_ptr);
  if (dv) {
    return dv->setVolume(volume);
  }
}