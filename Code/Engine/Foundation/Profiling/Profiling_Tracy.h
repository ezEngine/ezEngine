#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>
#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)
#  if defined(TRACY_ENABLE)
#    include <tracy/tracy/Tracy.hpp>

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const char* szString)
{
  return ezStringUtils::GetStringElementCount(szString);
}

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(ezStringView sString)
{
  return sString.GetElementCount();
}

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const ezString& sString)
{
  return sString.GetElementCount();
}
EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const ezStringBuilder& sString)
{
  return sString.GetElementCount();
}
EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const ezHashedString& sString)
{
  return sString.GetView().GetElementCount();
}
EZ_ALWAYS_INLINE const char* __convertezStringToConstChar(const ezString& sString)
{
  return sString.GetData();
}
EZ_ALWAYS_INLINE const char* __convertezStringToConstChar(const ezStringBuilder& sString)
{
  return sString.GetData();
}
EZ_ALWAYS_INLINE const char* __convertezStringToConstChar(const ezHashedString& sString)
{
  return sString.GetData();
}
EZ_ALWAYS_INLINE const char* __convertezStringToConstChar(const ezStringView& sString)
{
  // can just return the string views start pointer, because this is used together with ___tracyGetStringLength
  return sString.GetStartPointer();
}
/// Let the accepted types pass through.
EZ_ALWAYS_INLINE const char* __convertezStringToConstChar(const char* szString)
{
  return szString;
}

#    define TRACY_PROFILE_SCOPE_DYNAMIC(szScopeName) \
      ZoneScoped;                                    \
      ZoneName(__convertezStringToConstChar(szScopeName), ___tracyGetStringLength(szScopeName))

/// \brief Profiles the current scope using the given name.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingScope
/// \sa EZ_PROFILE_LIST_SCOPE
#    define EZ_PROFILE_SCOPE(szScopeName)                                                                                 \
      ezProfilingScope EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(szScopeName, EZ_SOURCE_FUNCTION, ezTime::MakeZero()); \
      TRACY_PROFILE_SCOPE_DYNAMIC(szScopeName)

/// \brief Same as EZ_PROFILE_SCOPE but if the scope takes longer than 'Timeout', the ezProfilingSystem's timeout callback is executed.
///
/// This can be used to log an error or save a callstack, etc. when a scope exceeds an expected amount of time.
///
/// \sa ezProfilingSystem::SetScopeTimeoutCallback()
#    define EZ_PROFILE_SCOPE_WITH_TIMEOUT(szScopeName, Timeout) \
      TRACY_PROFILE_SCOPE_DYNAMIC(szScopeName);
/// \brief Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use EZ_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope
/// section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_NEXT_SECTION
#    define EZ_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) \
      TRACY_PROFILE_SCOPE_DYNAMIC(szScopeName);

/// \brief Starts a new section in a EZ_PROFILE_LIST_SCOPE
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_SCOPE
#    define EZ_PROFILE_LIST_NEXT_SECTION(szNextSectionName) ezProfilingListScope::StartNextSection(szNextSectionName)

#    define EZ_PROFILER_END_FRAME FrameMark;
#  endif
#endif
