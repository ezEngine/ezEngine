#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

#if EZ_ENABLED(EZ_USE_PROFILING) && TRACY_ENABLE && !EZ_DOCS

#  include <tracy/tracy/Tracy.hpp>

EZ_ALWAYS_INLINE ezUInt32 __tracyEzStringLength(const char* szString)
{
  return ezStringUtils::GetStringElementCount(szString);
}

EZ_ALWAYS_INLINE ezUInt32 __tracyEzStringLength(ezStringView sString)
{
  return sString.GetElementCount();
}

EZ_ALWAYS_INLINE ezUInt32 __tracyEzStringLength(const ezString& sString)
{
  return sString.GetElementCount();
}

EZ_ALWAYS_INLINE ezUInt32 __tracyEzStringLength(const ezStringBuilder& sString)
{
  return sString.GetElementCount();
}

EZ_ALWAYS_INLINE ezUInt32 __tracyEzStringLength(const ezHashedString& sString)
{
  return sString.GetView().GetElementCount();
}

EZ_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const ezString& sString)
{
  return sString.GetData();
}

EZ_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const ezStringBuilder& sString)
{
  return sString.GetData();
}

EZ_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const ezHashedString& sString)
{
  return sString.GetData();
}

EZ_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const ezStringView& sString)
{
  // can just return the string views start pointer, because this is used together with __tracyEzStringLength
  return sString.GetStartPointer();
}

EZ_ALWAYS_INLINE const char* __tracyEzStringToConstChar(const char* szString)
{
  return szString;
}

/// \brief Similar to EZ_PROFILE_SCOPE, but only forwards to Tracy
#  define EZ_TRACY_PROFILE_SCOPE(ScopeName) \
    ZoneScoped;                             \
    ZoneName(__tracyEzStringToConstChar(ScopeName), __tracyEzStringLength(ScopeName))

// Override the standard EZ profiling macros and inject Tracy profiling scopes

#  undef EZ_PROFILE_SCOPE
#  define EZ_PROFILE_SCOPE(ScopeName)                                                                                 \
    ezProfilingScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ScopeName, EZ_SOURCE_FUNCTION, ezTime::MakeZero()); \
    EZ_TRACY_PROFILE_SCOPE(ScopeName)

#  undef EZ_PROFILE_SCOPE_WITH_TIMEOUT
#  define EZ_PROFILE_SCOPE_WITH_TIMEOUT(ScopeName, Timeout)                                                \
    ezProfilingScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ScopeName, EZ_SOURCE_FUNCTION, Timeout); \
    EZ_TRACY_PROFILE_SCOPE(ScopeName);

#  undef EZ_PROFILE_LIST_SCOPE
#  define EZ_PROFILE_LIST_SCOPE(ListName, FirstSectionName)                                                            \
    ezProfilingListScope EZ_PP_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(ListName, FirstSectionName, EZ_SOURCE_FUNCTION); \
    EZ_TRACY_PROFILE_SCOPE(ScopeName);

#  undef EZ_PROFILER_FRAME_MARKER
#  define EZ_PROFILER_FRAME_MARKER() FrameMark

#else

/// \brief Similar to EZ_PROFILE_SCOPE, but only forwards to Tracy
#  define EZ_TRACY_PROFILE_SCOPE(ScopeName)

#endif
