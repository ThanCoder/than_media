// Windows အတွက် Media Foundation Decoder ကို ဖွင့်ခြင်း
#define MA_SUPPORT_RESOURCE_MANAGER_WITH_MEDIA_FOUNDATION

// Apple (macOS/iOS) အတွက် Core Audio Decoder ကို ဖွင့်ခြ

#define MINIAUDIO_IMPLEMENTATION
#include "audio_player.hpp"

bool AudioPlayer::init(AudioErrorCallback error_callback) {
  // ma_engine_config config = ma_engine_config_init();

  ma_result result = ma_engine_init(nullptr, &engine);
  if (result != ma_result::MA_SUCCESS) {
    if (error_callback != nullptr) {
      error_callback("Failed to init engine");
    }
    return false;
  }
  is_engine_initialized = true;

  return true;
}
AudioPlayer::~AudioPlayer() {
  unload_file();

  if (is_engine_initialized) {
    is_engine_initialized = false;
    ma_engine_uninit(&engine);
  }
}

// load sound
bool AudioPlayer::load_file(const std::string& audio_path,
                            AudioErrorCallback error_callback) {
  ma_result result = ma_sound_init_from_file(&engine, audio_path.c_str(), 0,
                                             nullptr, nullptr, &sound);

  if (result != ma_result::MA_SUCCESS) {
    if (error_callback != nullptr) {
      error_callback("Failed to Load Audio: " + audio_path);
    }
    return false;
  }
  is_sound_loaded = true;
  return true;
}
void AudioPlayer::unload_file() {
  if (is_sound_loaded) {
    stop();
    ma_sound_uninit(&sound);
    is_sound_loaded = false;
  }
}

void AudioPlayer::play() {
  if (is_player_ready()) {
    ma_sound_start(&sound);
  }
}

void AudioPlayer::pause() {
  if (is_player_ready()) {
    ma_sound_stop(&sound);
  }
}

void AudioPlayer::stop() {
  if (is_player_ready()) {
    ma_sound_stop(&sound);
    seek(0);
  }
}

bool AudioPlayer::is_playing() { return ma_sound_is_playing(&sound); }

bool AudioPlayer::is_at_end() { return ma_sound_at_end(&sound); }

float AudioPlayer::get_duration_seconds() {
  float length = 0.0f;
  ma_sound_get_length_in_seconds(&sound, &length);
  return length;
}

float AudioPlayer::get_current_seconds() {
  float cursor = 0.0f;
  ma_sound_get_cursor_in_seconds(&sound, &cursor);
  return cursor;
}

void AudioPlayer::seek(float seconds) {
  ma_sound_seek_to_second(&sound, seconds);
}

void AudioPlayer::set_volume(float volume) {
  ma_sound_set_volume(&sound, volume);
}
float AudioPlayer::get_volume() { return ma_sound_get_volume(&sound); }

// loop
void AudioPlayer::set_looping(bool loop) { ma_sound_set_looping(&sound, loop); }
bool AudioPlayer::is_looping() { return ma_sound_is_looping(&sound); }

void AudioPlayer::set_fade_in_milliseconds(float volumeBeg, float volumeEnd,
                                           ma_uint64 fadeLengthInMilliseconds) {
  ma_sound_set_fade_in_milliseconds(&sound, volumeBeg, volumeEnd,
                                    fadeLengthInMilliseconds);
}

void AudioPlayer::set_fade_start_in_milliseconds(
    float volumeBeg, float volumeEnd, ma_uint64 fadeLengthInMilliseconds,
    ma_uint64 absoluteGlobalTimeInMilliseconds) {
  ma_sound_set_fade_start_in_milliseconds(&sound, volumeBeg, volumeEnd,

                                          fadeLengthInMilliseconds,
                                          absoluteGlobalTimeInMilliseconds);
}
void AudioPlayer::stop_with_fade_in_milliseconds(ma_uint64 fadeLengthInFrames) {
  ma_sound_stop_with_fade_in_milliseconds(&sound, fadeLengthInFrames);
}
void AudioPlayer::reset_stop_time_and_fade() {
  ma_sound_reset_stop_time_and_fade(&sound);
}
void AudioPlayer::reset_fade() { ma_sound_reset_fade(&sound); }

float AudioPlayer::get_current_fade_volume() {
  return ma_sound_get_current_fade_volume(&sound);
}

void AudioPlayer::set_stop_time_with_fade_in_milliseconds(
    ma_uint64 stopAbsoluteGlobalTimeInMilliseconds,
    ma_uint64 fadeLengthInMilliseconds) {
  ma_sound_set_stop_time_with_fade_in_milliseconds(
      &sound, stopAbsoluteGlobalTimeInMilliseconds, fadeLengthInMilliseconds);
}
