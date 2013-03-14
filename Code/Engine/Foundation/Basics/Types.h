#pragma once

// ***** Definition of types *****

typedef unsigned char     ezUInt8;
typedef unsigned short    ezUInt16;
typedef unsigned int      ezUInt32;
// ezUInt64 is defined in the compiler specific header

typedef char              ezInt8;
typedef short             ezInt16;
typedef int               ezInt32;
// ezInt64 is defined in the compiler specific header

// no float-types, since those are well portable

// Do some compile-time checks on the types
EZ_CHECK_AT_COMPILETIME(sizeof(bool)     == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(char)     == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(float)    == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(double)   == 8);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt8)   == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt16)  == 2);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt32)  == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt64)  == 8); // must be defined in the specific compiler header
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt8)  == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt16) == 2);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt32) == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) == 8); // must be defined in the specific compiler header

#if EZ_PLATFORM_64BIT
  #define EZ_ALIGNMENT_MINIMUM 8
#elif EZ_PLATFORM_32BIT
  #define EZ_ALIGNMENT_MINIMUM 4
#else
  #error "Unknown pointer size."
#endif

EZ_CHECK_AT_COMPILETIME(sizeof(void*) == EZ_ALIGNMENT_MINIMUM);

enum ezResultEnum
{
  EZ_FAILURE,
  EZ_SUCCESS
};

/// Default enum for returning failure or success, instead of using a bool.
struct ezResult
{
public:
  ezResult(ezResultEnum res) : e (res) {}

  void operator=  (ezResultEnum rhs) { e = rhs; }
  bool operator== (ezResultEnum cmp) const { return e == cmp; }
  bool operator!= (ezResultEnum cmp) const { return e != cmp; }

private:
  ezResultEnum e;
};

/// Alignment helper. Derive from this struct if alignment is depending on a template parameter.
/// If alignment is fixed always use the EZ_ALIGN macro.
template <size_t ALIGNMENT> struct ezAligned;
template <> struct EZ_ALIGN(ezAligned<1>, 1) { };
template <> struct EZ_ALIGN(ezAligned<2>, 2) { };
template <> struct EZ_ALIGN(ezAligned<4>, 4) { };
template <> struct EZ_ALIGN(ezAligned<8>, 8) { };
template <> struct EZ_ALIGN(ezAligned<16>, 16) { };
template <> struct EZ_ALIGN(ezAligned<32>, 32) { };
template <> struct EZ_ALIGN(ezAligned<64>, 64) { };
template <> struct EZ_ALIGN(ezAligned<128>, 128) { };
template <> struct EZ_ALIGN(ezAligned<256>, 256) { };
template <> struct EZ_ALIGN(ezAligned<512>, 512) { };
template <> struct EZ_ALIGN(ezAligned<1024>, 1024) { };
template <> struct EZ_ALIGN(ezAligned<2048>, 2048) { };
template <> struct EZ_ALIGN(ezAligned<4096>, 4096) { };
template <> struct EZ_ALIGN(ezAligned<8192>, 8192) { };
