#pragma once

#if defined(__clang__) || defined(__GNUC__)

#  if defined(__x86_64__) || defined(__i386__)
#    undef EZ_PLATFORM_ARCH_X86
#    define EZ_PLATFORM_ARCH_X86 EZ_ON
#  elif defined(__arm__) || defined(__aarch64__)
#    undef EZ_PLATFORM_ARCH_ARM
#    define EZ_PLATFORM_ARCH_ARM EZ_ON
#  elif (__EMSCRIPTEN__)
#    undef EZ_PLATFORM_ARCH_WEB
#    define EZ_PLATFORM_ARCH_WEB EZ_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(__x86_64__) || defined(__aarch64__)
#    undef EZ_PLATFORM_64BIT
#    define EZ_PLATFORM_64BIT EZ_ON
#  elif defined(__i386__) || defined(__arm__)
#    undef EZ_PLATFORM_32BIT
#    define EZ_PLATFORM_32BIT EZ_ON
#  elif (__EMSCRIPTEN__)
#    undef EZ_PLATFORM_32BIT
#    define EZ_PLATFORM_32BIT EZ_ON
#  else
#    error unhandled platform bit count
#  endif

#elif defined(_MSC_VER)

#  if defined(_M_AMD64) || defined(_M_IX86)
#    undef EZ_PLATFORM_ARCH_X86
#    define EZ_PLATFORM_ARCH_X86 EZ_ON
#  elif defined(_M_ARM) || defined(_M_ARM64)
#    undef EZ_PLATFORM_ARCH_ARM
#    define EZ_PLATFORM_ARCH_ARM EZ_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(_M_AMD64) || defined(_M_ARM64)
#    undef EZ_PLATFORM_64BIT
#    define EZ_PLATFORM_64BIT EZ_ON
#  elif defined(_M_IX86) || defined(_M_ARM)
#    undef EZ_PLATFORM_32BIT
#    define EZ_PLATFORM_32BIT EZ_ON
#  else
#    error unhandled platform bit count
#  endif

#else
#  error unhandled compiler
#endif
