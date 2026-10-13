#pragma once

#if defined(__GNUC__) || defined(__clang__)
  #define PACKED __attribute__((packed))
#else
  #error "Unsupported compiler. Specify packed attribute for compiler you use"
#endif

#if defined(__GNUC__) || defined(__clang__)
  #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    #error "This code was meant to work on little-endian machine only. So stop trying to run this code on power pc, it's 2026 already"
  #endif
#else
  #warning "Can't check endianness on unknown compiler. Assuming code is running on little-endian machine"
#endif
