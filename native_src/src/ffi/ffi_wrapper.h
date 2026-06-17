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

void* video_reader_create(const char* video_path_ptr);
bool video_reader_decode_next_frame(void* reader_ptr,
                                    unsigned char* output_buffer_ptr);
Video_Format_Info video_reader_get_format_info(void* reader_ptr);
const char* video_reader_get_codec_name(void* reader_ptr);
void video_reader_close(void* video_ptr);
void video_reader_destroy(void* reader_ptr);

#ifdef __cplusplus
}
#endif