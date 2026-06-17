// ignore_for_file: avoid_print

import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:native_toolchain_c/native_toolchain_c.dart';
import 'package:logging/logging.dart';
import 'package:hooks/hooks.dart';
import 'package:path/path.dart';

void main(List<String> args) async {
  await build(args, (input, output) async {
    final packageName = input.packageName;
    final source = join(input.packageRoot.toFilePath(), 'native_src');
    final src = join(source, 'src');
    final libso = join(
      input.packageRoot.toFilePath(),
      'build',
      'libmedia_app.so',
    );
    final cbuilder = CBuilder.library(
      name: packageName,
      assetName: '${packageName}_bindings_generated.dart',
      sources: [join(src, 'ffi_wrapper.cpp')],
      includes: [join(source, 'include')],
      language: .cpp,
    );
    output.assets.code.add(
      CodeAsset(
        package: packageName,
        name: packageName,
        linkMode: DynamicLoadingBundled(),
        file: File(libso).uri,
      ),
    );
    await cbuilder.run(
      input: input,
      output: output,
      logger: Logger('')
        ..level = .ALL
        ..onRecord.listen((record) => print(record.message)),
    );
    output.dependencies.add(File(libso).uri);
  });
}
