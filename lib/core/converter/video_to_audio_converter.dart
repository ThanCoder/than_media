// ignore_for_file: avoid_print, non_constant_identifier_names

import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:than_media/than_media.dart';

class VideoToAudioConverter {
  /// ### Video To Audio
  ///
  /// AUDIO_FORMAT_WAV(0),
  ///
  /// AUDIO_FORMAT_AAC(1),
  ///
  /// AUDIO_FORMAT_MP3(2);
  ///
  ///
  static Future<bool> convert(
    String videoPath,
    String outPath, {
    Audio_Format audioFormat = Audio_Format.AUDIO_FORMAT_AAC,
    void Function(double progress)? onProgress,
    bool Function()? checkCancelled,
  }) async {
    return await Isolate.run(() {
      final file_path = videoPath.toNativeUtf8();
      final out_path = outPath.toNativeUtf8();
      bool success = false;
      NativeCallable<Bool Function(Double)>? nativeCallback;

      bool isCancelled = false;
      final startTime = DateTime.now();

      if (onProgress != null) {
        nativeCallback = NativeCallable<Bool Function(Double)>.isolateLocal((
          double progress,
        ) {
          onProgress(progress);
          // 🎯 Loop ထဲကို ရောက်လာတိုင်း လက်ရှိအချိန်ကို စစ်တယ်
          final currentTime = DateTime.now();
          final difference = currentTime.difference(startTime).inSeconds;

          // ၃ စက္ကန့် ကျော်သွားပြီဆိုရင် true ပြောင်းပြီး C++ ကို false ပြန်မယ်
          if (difference >= 3) {
            isCancelled = true;
            print('3 seconds reached! Triggering isCancelled: $isCancelled');
          }
          return isCancelled ? false : true;
        }, exceptionalReturn: false);
      }

      Future.delayed(Duration(seconds: 3)).then((value) {
        isCancelled = true;
        print('call isCancelled: $isCancelled');
      });

      try {
        success = file_saver_save_as(
          file_path.cast<Char>(),
          out_path.cast<Char>(),
          audioFormat.value,
          nativeCallback != null ? nativeCallback.nativeFunction : nullptr,
        );
      } catch (e) {
        print('[VideoToAudioConverter:convert]: $e');
      } finally {
        calloc.free(file_path);
        calloc.free(out_path);
        if (nativeCallback != null) {
          nativeCallback.close();
        }
      }
      return success;
    });
  }
}
