// Windows အတွက် Media Foundation Decoder ကို ဖွင့်ခြင်း
#define MA_SUPPORT_RESOURCE_MANAGER_WITH_MEDIA_FOUNDATION

// Apple (macOS/iOS) အတွက် Core Audio Decoder ကို ဖွင့်ခြင်း

#define MINIAUDIO_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// C++ ရဲ့ Name Mangling ပြဿနာကို ဖြေရှင်းရန် extern "C" သုံးပေးရပါမည်
extern "C" {
#include "miniaudio.h"
}