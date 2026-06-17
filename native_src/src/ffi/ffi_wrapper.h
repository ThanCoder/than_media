#pragma once

#if defined(_WIN32)
#define FFI_EXPORT __declspec(dllexport)
#else
#define FFI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>

#endif
typedef struct {
  int width;
  int height;
  double fps;
  double durationSeconds;

} Video_Format_Info;
typedef enum {
  AUDIO_FORMAT_WAV,
  AUDIO_FORMAT_AAC,
  AUDIO_FORMAT_MP3
} Audio_Format;
///*********************Video Reader********************************* */
void* video_reader_create(const char* video_path_ptr);
bool video_reader_decode_next_frame(void* reader_ptr,
                                    unsigned char* output_buffer_ptr);
Video_Format_Info video_reader_get_format_info(void* reader_ptr);
const char* video_reader_get_codec_name(void* reader_ptr);
void video_reader_close(void* video_ptr);
void video_reader_destroy(void* reader_ptr);

///*********************Audio Device********************************* */
void* audio_device_create(const char* file_path);
void audio_device_destroy(void* audio_device_ptr);
bool audio_device_init(void* audio_device_ptr);
bool audio_device_start(void* audio_device_ptr);
void audio_device_stop(void* audio_device_ptr);
void audio_device_uninit(void* audio_device_ptr);
double audio_device_getCurrentInSeconds(void* audio_device_ptr);
double audio_device_getDurationInSeconds(void* audio_device_ptr);
void audio_device_seek(void* audio_device_ptr, double seconds);
void audio_device_setVolume(void* audio_device_ptr, float volume);
///*********************File Saver********************************* */
bool file_saver_saveAsWav(void* media_file_ptr, const char* outPath);
bool file_saver_saveAsAac(void* media_file_ptr, const char* outPath);
bool file_saver_saveAsMp3(void* media_file_ptr, const char* outPath);
// Audio_Format::AUDIO_FORMAT_WAV
bool file_saver_save_as(void* media_file_ptr, const char* outPath, int format);
///*********************Media File********************************* */
void* media_file_create(const char* file_path);
bool media_file_openFile(void* media_file_ptr);
bool media_file_readNextAudioChunk(void* media_file_ptr,
                                   unsigned char* out_chunk);
bool media_file_seekToSeconds(void* media_file_ptr, double seconds);
double media_file_getDurationInSeconds(void* media_file_ptr);
double media_file_getCurrentInSeconds(void* media_file_ptr);
const char* media_file_getMetadata(void* media_file_ptr, const char* key);
unsigned char* media_file_getAlbumArt(void* media_file_ptr);

// ၃။ တခြား Class တွေက လာမေးမယ့် Getter Functions
// AudioFormatInfo getTargetFormat(void* media_file_ptr);
const char* media_file_getCurrentPath(void* media_file_ptr);
void media_file_closeFile(void* media_file_ptr);
unsigned char* media_file_getVideoThumbnail(void* media_file_ptr,
                                            double seconds, int targetWidth = 0,
                                            int targetHeight = 0);

#ifdef __cplusplus
}
#endif