#pragma once

// ***** Definition of types *****

typedef unsigned char ezUInt8;
typedef unsigned short ezUInt16;
typedef unsigned int ezUInt32;
typedef unsigned long long ezUInt64;

typedef signed char ezInt8;
typedef short ezInt16;
typedef int ezInt32;
typedef long long ezInt64;

// no float-types, since those are well portable

// Do some compile-time checks on the types
EZ_CHECK_AT_COMPILETIME(sizeof(bool) == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(char) == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(float) == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(double) == 8);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt8) == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt16) == 2);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt32) == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(ezInt64) == 8); // must be defined in the specific compiler header
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt8) == 1);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt16) == 2);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt32) == 4);
EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) == 8); // must be defined in the specific compiler header
EZ_CHECK_AT_COMPILETIME(sizeof(long long int) == 8);

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
#define EZ_ALIGNMENT_MINIMUM 8
#elif EZ_ENABLED(EZ_PLATFORM_32BIT)
#define EZ_ALIGNMENT_MINIMUM 4
#else
#error "Unknown pointer size."
#endif

EZ_CHECK_AT_COMPILETIME(sizeof(void*) == EZ_ALIGNMENT_MINIMUM);

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
enum ezResultEnum
{
  EZ_FAILURE,
  EZ_SUCCESS
};

/// \brief Default enum for returning failure or success, instead of using a bool.
struct ezResult
{
public:
  ezResult(ezResultEnum res)
      : e(res)
  {
  }

  void operator=(ezResultEnum rhs) { e = rhs; }
  bool operator==(ezResultEnum cmp) const { return e == cmp; }
  bool operator!=(ezResultEnum cmp) const { return e != cmp; }

  EZ_ALWAYS_INLINE bool Succeeded() const { return e == EZ_SUCCESS; }
  EZ_ALWAYS_INLINE bool Failed() const { return e == EZ_FAILURE; }
  EZ_ALWAYS_INLINE void IgnoreResult() { /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */ }

private:
  ezResultEnum e;
};

/// \brief Explicit conversion to ezResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
inline ezResult ezToResult(ezResult result)
{
  return result;
}

/// \brief Helper macro to call functions that return ezStatus or ezResult in a function that returns ezStatus (or ezResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define EZ_SUCCEED_OR_RETURN(code)                                                                                                         \
  do                                                                                                                                       \
  {                                                                                                                                        \
    auto s = (code);                                                                                                                       \
    if (ezToResult(s).Failed())                                                                                                            \
      return s;                                                                                                                            \
  } while (false)

/// \brief Like EZ_SUCCEED_OR_RETURN, but with error logging.
#define EZ_SUCCEED_OR_RETURN_LOG(code)                                                                                                     \
  do                                                                                                                                       \
  {                                                                                                                                        \
    auto s = (code);                                                                                                                       \
    if (ezToResult(s).Failed())                                                                                                            \
    {                                                                                                                                      \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_STRINGIZE(code), s);                                                                  \
      return s;                                                                                                                            \
    }                                                                                                                                      \
  } while (false)

/// \brief Like EZ_SUCCEED_OR_RETURN, but with custom error logging.
#define EZ_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                                                                                         \
  do                                                                                                                                       \
  {                                                                                                                                        \
    auto s = (code);                                                                                                                       \
    if (ezToResult(s).Failed())                                                                                                            \
    {                                                                                                                                      \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_STRINGIZE(code), log);                                                                \
      return s;                                                                                                                            \
    }                                                                                                                                      \
  } while (false)

//////////////////////////////////////////////////////////////////////////

class ezRTTI;

/// \brief Dummy type to pass to templates and macros that expect a base type for a class that has no base.
class ezNoBase
{
public:
  static const ezRTTI* GetStaticRTTI() { return nullptr; }
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an enum class.
class ezEnumBase
{
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an bitflags class.
class ezBitflagsBase
{
};

/// \brief Alignment helper. Derive from this struct if alignment is depending on a template parameter.
/// If alignment is fixed always use the EZ_ALIGN macro.
template <size_t Alignment>
struct ezAligned;
template <>
struct EZ_ALIGN(ezAligned<1>, 1)
{
};
template <>
struct EZ_ALIGN(ezAligned<2>, 2)
{
};
template <>
struct EZ_ALIGN(ezAligned<4>, 4)
{
};
template <>
struct EZ_ALIGN(ezAligned<8>, 8)
{
};
template <>
struct EZ_ALIGN(ezAligned<16>, 16)
{
};
template <>
struct EZ_ALIGN(ezAligned<32>, 32)
{
};
template <>
struct EZ_ALIGN(ezAligned<64>, 64)
{
};
template <>
struct EZ_ALIGN(ezAligned<128>, 128)
{
};
template <>
struct EZ_ALIGN(ezAligned<256>, 256)
{
};
template <>
struct EZ_ALIGN(ezAligned<512>, 512)
{
};
template <>
struct EZ_ALIGN(ezAligned<1024>, 1024)
{
};
template <>
struct EZ_ALIGN(ezAligned<2048>, 2048)
{
};
template <>
struct EZ_ALIGN(ezAligned<4096>, 4096)
{
};
template <>
struct EZ_ALIGN(ezAligned<8192>, 8192)
{
};


/// \brief Helper struct to get a storage type from a size in byte.
template <size_t SizeInByte>
struct ezSizeToType;
/// \cond
template <>
struct ezSizeToType<1>
{
  typedef ezUInt8 Type;
};
template <>
struct ezSizeToType<2>
{
  typedef ezUInt16 Type;
};
template <>
struct ezSizeToType<3>
{
  typedef ezUInt32 Type;
};
template <>
struct ezSizeToType<4>
{
  typedef ezUInt32 Type;
};
template <>
struct ezSizeToType<5>
{
  typedef ezUInt64 Type;
};
template <>
struct ezSizeToType<6>
{
  typedef ezUInt64 Type;
};
template <>
struct ezSizeToType<7>
{
  typedef ezUInt64 Type;
};
template <>
struct ezSizeToType<8>
{
  typedef ezUInt64 Type;
};
/// \endcond

