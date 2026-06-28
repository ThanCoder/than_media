import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:hooks/hooks.dart';
import 'package:path/path.dart';

void main(List<String> args) async {
  await build(args, (input, output) async {
    final packageName = input.packageName;
    final targetOS = input.config.code.targetOS;
    final targetArchitecture = input.config.code.targetArchitecture;
    final libSourcePath = join(
      input.packageRoot.toFilePath(),
      'src',
      'dist_binaries',
    );
    final libName = 'libthan_media.so';

    File? libFile;
    if (targetOS == .linux) {
      libFile = File(join(libSourcePath, 'linux', libName));
    }
    if (targetOS == .android) {
      if (targetArchitecture == .arm) {
        libFile = File(join(libSourcePath, 'android', 'arm64-v8a', libName));
      }
      if (targetArchitecture == .arm64) {
        libFile = File(join(libSourcePath, 'android', 'armeabi-v7a', libName));
      }
    }

    if (libFile == null) {
      throw UnsupportedError(
        'Unsupported: `${targetOS.name}`-`${targetArchitecture.name}`',
      );
    }

    output.assets.code.add(
      CodeAsset(
        package: packageName,
        name: '${packageName}_bindings_generated.dart',
        linkMode: DynamicLoadingBundled(),
        file: libFile.uri,
      ),
    );
  });
}
