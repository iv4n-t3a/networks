#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define PACKED __attribute__((packed))
#else
#error "Unsupported compiler. Specify packed attribute for compiler you use"
#endif
