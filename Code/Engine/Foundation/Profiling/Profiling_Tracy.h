#include <Foundation/Basics.h>
#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)
#  if defined(TRACY_ENABLE)
#    include <tracy/tracy/Tracy.hpp>

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const char* szString)
{
  return ezStringUtils::GetStringElementCount(szString);
}

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(ezStringView szString)
{
  return szString.GetElementCount();
}

EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const ezString& szString)
{
  return szString.GetElementCount();
}
EZ_ALWAYS_INLINE ezUInt32 ___tracyGetStringLength(const ezStringBuilder& szString)
{
  return szString.GetElementCount();
}

#    define TRACY_PROFILE_SCOPE_DYNAMIC(szScopeName) \
      ZoneScoped;                                    \
      ZoneName(szScopeName, ___tracyGetStringLength(szScopeName))


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

#    define EZ_TRACY_END_FRAME FrameMark;
#endif
#endif
