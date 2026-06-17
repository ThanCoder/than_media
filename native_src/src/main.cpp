#include <atomic>
#include <chrono>
#define MINIAUDIO_IMPLEMENTATION  // <- ဒါကို အရင်ဆုံး လုံးဝ ထိပ်မှာ ထားရပါမယ်
#include <cstring>
#include <iostream>
#include <thread>

#include "audio_device.hpp"

// main.cpp ရဲ့ ထိပ်ဆုံးမှာ ဒီအတိုင်း ရေးပေးပါ

int main() {
  // Class instance တစ်ခု ဆောက်လိုက်ရုံရုံပါပဲ
  AudioDevice player("/home/thancoder/Music/Ko_Feel_ကိုဖီလ်း_Don_t_Go(256k).mp3");

  // Init လုပ်မယ်
  if (!player.init()) {
    return 1;
  }

  // Play မယ်
  std::cout << "Raw PCM ဖိုင်ကို ဖွင့်နေပါပြီ... ရပ်တန့်ရန် Enter နှိပ်ပါ။" << std::endl;
  if (!player.start()) {
    std::cerr << "Error: Audio Playback စတင်လို့မရပါ!" << std::endl;
    return -1;
  }

  std::atomic<bool> keep_monitor{true};

  std::thread monitor_thread([&player, &keep_monitor]() {
    while (keep_monitor) {
      std::cout << "Duration: " << player.getDurationInSeconds() << "s \n";
      std::cout << "Current: " << player.getCurrentInSeconds() << "s \n";

      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
  std::cout << "press to seek 30s\n";
  std::cin.get();
  player.seek(30);

  std::cout << "press to Stop" << std::endl;
  std::cin.get();
  player.stop();

  std::cout << "press to Play " << std::endl;
  std::cin.get();
  player.start();

  std::cout << "ထွက်ခွာရန် Enter နှိပ်ပါ။" << std::endl;
  std::cin.get();

  keep_monitor = false;
  if (monitor_thread.joinable()) {
    monitor_thread.join();
  }

  return 0;
}