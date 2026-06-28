// ignore_for_file: non_constant_identifier_names

import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:than_media/than_media_bindings_generated.dart';

part 'audio_device_state.dart';
part 'audio_device_state_handler.dart';

class AudioDevice with AudioDeviceStateHandler {
  @override
  Pointer<Void> _audio_device_ptr = nullptr;
  bool _initialized = false;
  bool get initialized => _initialized;

  Future<bool> init(String filePath) async {
    if (initialized) return false;
    final audioAddress = await Isolate.run<int?>(() {
      final file_path = filePath.toNativeUtf8();
      final ptr = audio_device_create(file_path.cast<Char>());
      final isInitialized = audio_device_init(ptr);

      // free
      calloc.free(file_path);

      return isInitialized ? ptr.address : null;
    });
    if (audioAddress == null) {
      // Duration ကို Init လုပ်ပြီးချင်း တစ်ခါတည်း ဆွဲထုတ်ပြီး Stream ထဲ ထည့်ပေးလိုက်တယ်
      final totalDuration = audio_device_getDurationInSeconds(
        _audio_device_ptr,
      );
      _durationController.add(totalDuration);

      _updateState(PlayerState.stopped);
      return false;
    }
    _audio_device_ptr = Pointer<Void>.fromAddress(audioAddress);
    _initialized = true;
    return true;
  }

  void start() {
    if (_audio_device_ptr == nullptr) return;
    final success = audio_device_start(_audio_device_ptr);
    if (success) {
      _updateState(PlayerState.playing);
      _startPositionTimer(); // Position ကို အချိန်နဲ့အမျှ Update လုပ်ဖို့ Timer မောင်းမယ်
    }
  }

  void stop() {
    if (_audio_device_ptr == nullptr) return;
    audio_device_stop(_audio_device_ptr);
    _updateState(PlayerState.paused);
    _stopPositionTimer();
    seek(0);
  }

  void pause() {
    if (_audio_device_ptr == nullptr) return;
    audio_device_stop(_audio_device_ptr);
    _updateState(PlayerState.paused);
    _stopPositionTimer();
  }

  void setVolume(double volume) {
    if (_audio_device_ptr == nullptr) return;
    audio_device_setVolume(_audio_device_ptr, volume.toDouble());
  }

  void seek(double seconds) {
    if (_audio_device_ptr == nullptr) return;
    audio_device_seek(_audio_device_ptr, seconds.toDouble());
    _positionController.add(seconds);
  }

  double get currentPosition {
    if (_audio_device_ptr == nullptr) return 0.0;
    return audio_device_getCurrentInSeconds(_audio_device_ptr);
  }

  double get duration {
    if (_audio_device_ptr == nullptr) return 0.0;
    return audio_device_getDurationInSeconds(_audio_device_ptr);
  }

  void destroy() {
    if (_audio_device_ptr == nullptr) return;
    audio_device_uninit(_audio_device_ptr);
    audio_device_destroy(_audio_device_ptr);
    _audio_device_ptr = nullptr;
    _initialized = false;
  }
}
