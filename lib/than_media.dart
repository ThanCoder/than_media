// ignore_for_file: avoid_print, non_constant_identifier_names

import 'dart:ffi';
import 'dart:isolate';

import 'package:ffi/ffi.dart';
import 'package:than_media/than_media_bindings_generated.dart';

export 'than_media_bindings_generated.dart';

Future<bool> saveAsVideoThumbnail(
  String filePath,
  String outPath, {
  required Duration duration,
  int targetWidth = 0,
  int targetHeight = 0,
}) async {
  return Isolate.run(() {
    final video_path = filePath.toNativeUtf8();
    final out_path = outPath.toNativeUtf8();
    bool saved = false;
    try {
      saved = media_file_saveAsVideoThumbnail(
        video_path.cast<Char>(),
        out_path.cast<Char>(),
        duration.inSeconds.toDouble(),
        targetWidth,
        targetHeight,
      );
    } catch (e) {
      print('[saveAsVideoThumbnail]: $e');
    }

    calloc.free(video_path);
    calloc.free(out_path);

    return saved;
  });
}
