#pragma once
#if defined(_WIN32)
#define FFI_EXPORT __declspec(dllexport)
#else
#define FFI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ffi_sum(int a,int b);

#ifdef __cplusplus
}
#endif