#pragma once

// ***** Definition of types *****

#include <cstdint>

using ezUInt8 = uint8_t;
using ezUInt16 = uint16_t;
using ezUInt32 = uint32_t;
using ezUInt64 = unsigned long long;

using ezInt8 = int8_t;
using ezInt16 = int16_t;
using ezInt32 = int32_t;
using ezInt64 = long long;

// no float-types, since those are well portable

// Do some compile-time checks on the types
static_assert(sizeof(bool) == 1);
static_assert(sizeof(char) == 1);
static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);
static_assert(sizeof(ezInt8) == 1);
static_assert(sizeof(ezInt16) == 2);
static_assert(sizeof(ezInt32) == 4);
static_assert(sizeof(ezInt64) == 8); // must be defined in the specific compiler header
static_assert(sizeof(ezUInt8) == 1);
static_assert(sizeof(ezUInt16) == 2);
static_assert(sizeof(ezUInt32) == 4);
static_assert(sizeof(ezUInt64) == 8); // must be defined in the specific compiler header
static_assert(sizeof(long long int) == 8);

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
#  define EZ_ALIGNMENT_MINIMUM 8
#elif EZ_ENABLED(EZ_PLATFORM_32BIT)
#  define EZ_ALIGNMENT_MINIMUM 4
#else
#  error "Unknown pointer size."
#endif

static_assert(sizeof(void*) == EZ_ALIGNMENT_MINIMUM);
static_assert(alignof(void*) == EZ_ALIGNMENT_MINIMUM);

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
    : m_E(res)
  {
  }

  void operator=(ezResultEnum rhs) { m_E = rhs; }
  bool operator==(ezResultEnum cmp) const { return m_E == cmp; }
  bool operator!=(ezResultEnum cmp) const { return m_E != cmp; }

  [[nodiscard]] EZ_ALWAYS_INLINE bool Succeeded() const { return m_E == EZ_SUCCESS; }
  [[nodiscard]] EZ_ALWAYS_INLINE bool Failed() const { return m_E == EZ_FAILURE; }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  EZ_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message. If \a details is provided, \a msg should contain a formatting element ({}), e.g. "Error: {}".
  void AssertSuccess(const char* szMsg = nullptr, const char* szDetails = nullptr) const;

private:
  ezResultEnum m_E;
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
#define EZ_SUCCEED_OR_RETURN_LOG(code)                                       \
  do                                                                         \
  {                                                                          \
    auto s = (code);                                                         \
    if (ezToResult(s).Failed())                                              \
    {                                                                        \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), s); \
      return s;                                                              \
    }                                                                        \
  } while (false)

/// \brief Like EZ_SUCCEED_OR_RETURN, but with custom error logging.
#define EZ_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                             \
  do                                                                           \
  {                                                                            \
    auto s = (code);                                                           \
    if (ezToResult(s).Failed())                                                \
    {                                                                          \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), log); \
      return s;                                                                \
    }                                                                          \
  } while (false)

//////////////////////////////////////////////////////////////////////////

class ezRTTI;
class ezAllocator;

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
