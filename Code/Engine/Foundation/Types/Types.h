#pragma once

// ***** Definition of types *****

using ezUInt8 = unsigned char;
using ezUInt16 = unsigned short;
using ezUInt32 = unsigned int;
using ezUInt64 = unsigned long long;

using ezInt8 = signed char;
using ezInt16 = short;
using ezInt32 = int;
using ezInt64 = long long;

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
#  define EZ_ALIGNMENT_MINIMUM 8
#elif EZ_ENABLED(EZ_PLATFORM_32BIT)
#  define EZ_ALIGNMENT_MINIMUM 4
#else
#  error "Unknown pointer size."
#endif

EZ_CHECK_AT_COMPILETIME(sizeof(void*) == EZ_ALIGNMENT_MINIMUM);

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
enum ezResultEnum
{
  EZ_FAILURE,
  EZ_SUCCESS
};

/// \brief Default enum for returning failure or success, instead of using a bool.
struct [[nodiscard]] EZ_FOUNDATION_DLL ezResult
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

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  EZ_ALWAYS_INLINE void IgnoreResult()
  { /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message. If \a details is provided, \a msg should contain a formatting element ({}), e.g. "Error: {}".
  void AssertSuccess(const char* msg = nullptr, const char* details = nullptr) const;

private:
  ezResultEnum e;
};

/// \brief Explicit conversion to ezResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
EZ_ALWAYS_INLINE ezResult ezToResult(ezResult result)
{
  return result;
}

/// \brief Helper macro to call functions that return ezStatus or ezResult in a function that returns ezStatus (or ezResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define EZ_SUCCEED_OR_RETURN(code) \
  do                               \
  {                                \
    auto s = (code);               \
    if (ezToResult(s).Failed())    \
      return s;                    \
  } while (false)

/// \brief Like EZ_SUCCEED_OR_RETURN, but with error logging.
#define EZ_SUCCEED_OR_RETURN_LOG(code)                                    \
  do                                                                      \
  {                                                                       \
    auto s = (code);                                                      \
    if (ezToResult(s).Failed())                                           \
    {                                                                     \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_STRINGIZE(code), s); \
      return s;                                                           \
    }                                                                     \
  } while (false)

/// \brief Like EZ_SUCCEED_OR_RETURN, but with custom error logging.
#define EZ_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                          \
  do                                                                        \
  {                                                                         \
    auto s = (code);                                                        \
    if (ezToResult(s).Failed())                                             \
    {                                                                       \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_STRINGIZE(code), log); \
      return s;                                                             \
    }                                                                       \
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

/// \brief Helper struct to get a storage type from a size in byte.
template <size_t SizeInByte>
struct ezSizeToType;
/// \cond
template <>
struct ezSizeToType<1>
{
  using Type = ezUInt8;
};
template <>
struct ezSizeToType<2>
{
  using Type = ezUInt16;
};
template <>
struct ezSizeToType<3>
{
  using Type = ezUInt32;
};
template <>
struct ezSizeToType<4>
{
  using Type = ezUInt32;
};
template <>
struct ezSizeToType<5>
{
  using Type = ezUInt64;
};
template <>
struct ezSizeToType<6>
{
  using Type = ezUInt64;
};
template <>
struct ezSizeToType<7>
{
  using Type = ezUInt64;
};
template <>
struct ezSizeToType<8>
{
  using Type = ezUInt64;
};
/// \endcond
