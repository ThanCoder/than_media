import 'package:flutter/material.dart';
import 'package:than_media/than_media.dart';

void main() {
  runApp(MaterialApp(home: const MyApp()));
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      floatingActionButton: FloatingActionButton(
        onPressed: () async {
          final videoPath =
              "/home/thancoder/Videos/A  Were wolf Boy (2026).mp4";
          final saved = await saveAsVideoThumbnail(
            videoPath,
            '../were_wolf.png',
            duration: Duration(seconds: 1),
          );
          print('saved: $saved');
        },
      ),
    );
  }
}
