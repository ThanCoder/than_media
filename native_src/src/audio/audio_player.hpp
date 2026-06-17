#pragma once

#include <functional>
#include <string>

extern "C" {
#include "miniaudio.h"
}

using AudioErrorCallback = std::function<void(const std::string message)>;

class AudioPlayer {
 private:
  ma_engine engine;
  ma_sound sound;
  // State Flags (Crash မဖြစ်အောင် ကာကွယ်ဖို့)
  bool is_engine_initialized = false;
  bool is_sound_loaded = false;

 public:
  AudioPlayer() = default;
  ~AudioPlayer();
  bool init(AudioErrorCallback callback = nullptr);
  bool load_file(const std::string &audio_path,
                 AudioErrorCallback callback = nullptr);
  void unload_file();  // Memory Leak မဖြစ်အောင် သီချင်းအဟောင်းကို ပိတ်မယ့် function

  // Helper standard getters
  bool is_player_ready() const {
    return is_engine_initialized && is_sound_loaded;
  }
  // Setter
  void play();
  void pause();
  void set_volume(float volume);
  float get_volume();
  void stop();
  bool is_playing();
  bool is_at_end();
  float get_current_seconds();
  float get_duration_seconds();
  void seek(float seconds);
  // loop
  void set_looping(bool loop);
  bool is_looping();
  // extra methods
  void set_fade_start_in_milliseconds(
      float volumeBeg, float volumeEnd, ma_uint64 fadeLengthInMilliseconds,
      ma_uint64 absoluteGlobalTimeInMilliseconds);
  void set_fade_in_milliseconds(float volumeBeg, float volumeEnd,
                                ma_uint64 fadeLengthInMilliseconds);
  void stop_with_fade_in_milliseconds(ma_uint64 fadeLengthInFrames);
  void reset_stop_time_and_fade();
  void reset_fade();
  float get_current_fade_volume();
  void set_stop_time_with_fade_in_milliseconds(
      ma_uint64 stopAbsoluteGlobalTimeInMilliseconds,
      ma_uint64 fadeLengthInMilliseconds);
};
