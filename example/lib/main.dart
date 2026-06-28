import 'package:flutter/material.dart';
import 'package:than_media/than_media.dart';

void main() {
  runApp(MaterialApp(home: const MyApp()));
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final audioDevice = AudioDevice();

  @override
  void initState() {
    audioDevice.stateStream.listen((event) {
      print('stateStream: $event');
    });
    audioDevice.positionStream.listen((event) {
      print('positionStream: $event');
    });
    super.initState();
  }

  @override
  void dispose() {
    audioDevice.destroy();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            TextButton(
              onPressed: () async {
                await audioDevice.init(
                  '/home/thancoder/Music/Han Tun - နင်စိတ်တိုင်းကျ.mp3',
                );
                print('init');
              },
              child: Text('init'),
            ),
            TextButton(
              onPressed: () {
                audioDevice.start();
              },
              child: Text('play'),
            ),
            TextButton(
              onPressed: () {
                audioDevice.pause();
              },
              child: Text('pause'),
            ),

            TextButton(
              onPressed: () {
                audioDevice.stop();
              },
              child: Text('stop'),
            ),
          ],
        ),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () async {
          bool isConvertCancel = false;

          final videoPath = '/home/thancoder/Videos/Supernatural S1/21.mp4';
          // final videoPath = "/home/thancoder/Videos/china.mp4";
          VideoToAudioConverter.convert(
            videoPath,
            '../21.aac',
            checkCancelled: () => isConvertCancel,
            onProgress: (progress) {
              print('progress: ${(progress * 100).toStringAsFixed(2)}%');
            },
          );
          Future.delayed(Duration(seconds: 3)).then((value) {
            isConvertCancel = true;
            print('cancel လိုက်ပြီ');
          });
          // final saved = await saveAsVideoThumbnail(
          //   videoPath,
          //   '../china.png',
          //   duration: Duration(seconds: 1),
          // );
        },
      ),
    );
  }
}
