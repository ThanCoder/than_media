#include <cstdint>
#include <fstream>
#include <vector>

#include "ffi_wrapper.h"
#include "video_reader.hpp"
// #define MINIAUDIO_IMPLEMENTATION  // <- ဒါကို အရင်ဆုံး လုံးဝ ထိပ်မှာ ထားရပါမယ်
#include <SDL2/SDL.h>

#include <cstring>
#include <iostream>

bool writeFile(const std::vector<uint8_t>& buff, const std::string output) {
  if (buff.empty()) {
    std::cerr << "Empty Vector!\n";
    return false;
  }
  std::ofstream outFile(output, std::ios::out | std::ios::binary);

  if (!outFile.is_open()) {
    std::cerr << "Open Failed!.Path: " << output << "\n";
    return false;
  }
  outFile.write(reinterpret_cast<const char*>(buff.data()), buff.size());
  outFile.close();

  return true;
}

void playVideo(VideoReader& video) {
  auto info = video.getFormatInfo();
  // SDL Windows တည်ဆောက်ခြင်း
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window =
      SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED, info.width, info.height, 0);
  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture* texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                        SDL_TEXTUREACCESS_STREAMING, info.width, info.height);

  std::vector<uint8_t> pixelBuffer;
  bool running = true;
  SDL_Event event;

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
    }

    // ဗီဒီယို Class ဆီကနေ Frame ကို သန့်သန့်ရှင်းရှင်း တောင်းယူတယ်
    if (video.readNextFrame(pixelBuffer)) {
      SDL_UpdateTexture(texture, nullptr, pixelBuffer.data(), info.width * 3);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);

      // Frame Rate အလိုက် စောင့်ဆိုင်းခြင်း
      SDL_Delay(static_cast<Uint32>(1000.0 / info.fps));
    } else {
      SDL_Delay(10);  // ဖိုင်ဆုံးရင် ငြိမ်နေမယ်
    }
  }

  // Cleanup
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
// ffmpeg-8.1.1
int main() {
  VideoReader video("/home/thancoder/Videos/adehhh-sunset.mp4");

  if (!video.openFile()) {
    return -1;
  }

  VideoFormatInfo info = video.getFormatInfo();
  std::cout << "Playing: " << info.width << "x" << info.height
            << " @ "
               "Codec: "
            << info.codecName << " " << info.fps << " FPS\n";
  // playVideo(video);
  // wrapper test
  auto vd = video_reader_create("/home/thancoder/Videos/adehhh-sunset.mp4");
  auto pl = reinterpret_cast<VideoReader*>(vd);

  // playVideo(*pl);
  std::cout << "info: " << video_reader_get_codec_name(pl) << "\n";

  video_reader_destroy(vd);

  return 0;
}