#include <Foundation/PCH.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>

const char* ezPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (ezStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    ezUnicodeUtils::MoveToPriorUtf8(szStartSearchAt);

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool ezPathUtils::HasAnyExtension(const char* szPath, const char* szPathEnd)
{
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szDot = ezStringUtils::FindLastSubString(szPath, ".", nullptr, szPathEnd);

  if (szDot == nullptr)
    return false;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  return (szSeparator < szDot);
}

bool ezPathUtils::HasExtension(const char* szPath, const char* szExtension, const char* szPathEnd)
{
  if (ezStringUtils::StartsWith(szExtension, "."))
    return ezStringUtils::EndsWith_NoCase(szPath, szExtension, szPathEnd);

  ezStringBuilder sExt;
  sExt.Append(".", szExtension);

  return ezStringUtils::EndsWith_NoCase(szPath, sExt.GetData(), szPathEnd);
}

ezStringIterator ezPathUtils::GetFileExtension(const char* szPath, const char* szPathEnd)
{
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szDot = ezStringUtils::FindLastSubString(szPath, ".", nullptr, szPathEnd);

  if (szDot == nullptr)
    return ezStringIterator(nullptr);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  if (szSeparator > szDot)
    return ezStringIterator(nullptr);

  return ezStringIterator(szDot + 1, szPathEnd, szDot + 1);
}

ezStringIterator ezPathUtils::GetFileNameAndExtension(const char* szPath, const char* szPathEnd)
{
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  if (szSeparator == nullptr)
    return ezStringIterator(szPath, szPathEnd, szPath);

  return ezStringIterator(szSeparator + 1, szPathEnd, szSeparator + 1);
}

ezStringIterator ezPathUtils::GetFileName(const char* szPath, const char* szPathEnd)
{
  // make sure szPathEnd is valid
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  const char* szDot = ezStringUtils::FindLastSubString(szPath, ".", szPathEnd);

  if (szDot < szSeparator) // includes (szDot == nullptr), szSeparator will never be nullptr here -> no extension
  {
    return ezStringIterator(szSeparator + 1, szPathEnd, szSeparator + 1);
  }

  if (szSeparator == nullptr)
  {
    if (szDot == nullptr) // no folder, no extension -> the entire thing is just a name
      return ezStringIterator(szPath, szPathEnd, szPath);

    return ezStringIterator(szPath, szDot, szPath); // no folder, but an extension -> remove the extension
  }

  // now: there is a separator AND an extension

  return ezStringIterator(szSeparator + 1, szDot, szSeparator + 1);
}

ezStringIterator ezPathUtils::GetFileDirectory(const char* szPath, const char* szPathEnd)
{
  // make sure szPathEnd is valid
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  ezStringIterator end (szPath, szPathEnd, szPath);
  end.ResetToBack();

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(end.GetCharacter()))
    return ezStringIterator(szPath, szPathEnd, szPath);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return ezStringIterator(nullptr);

  return ezStringIterator(szPath, szSeparator + 1, szPath);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const char ezPathUtils::OsSpecificPathSeparator = '\\';
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  const char ezPathUtils::OsSpecificPathSeparator = '/';
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  const char ezPathUtils::OsSpecificPathSeparator = '/';
#else
  #error Unknown platform.
#endif

bool ezPathUtils::IsAbsolutePath(const char* szPath)
{
  if (ezStringUtils::IsNullOrEmpty (szPath))
    return false;

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    /// if it is an absolute path, character 0 must be ASCII (A - Z)
    /// checks for local paths, ie. 'C:\stuff' and UNC paths, ie. '\\server\stuff'
    /// not sure if we should handle '//' identical to '\\' (currently we do)
    return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
  #elif EZ_ENABLED(EZ_PLATFORM_LINUX)
    return (szPath[0] == '/');
  #elif EZ_ENABLED(EZ_PLATFORM_OSX)
    return (szPath[0] == '/');
  #else
    #error Unknown platform.
  #endif
}

bool ezPathUtils::IsRelativePath(const char* szPath)
{
  if (ezStringUtils::IsNullOrEmpty(szPath))
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (ezPathUtils::IsPathSeparator(szPath[0]))
    return false;

  return !IsAbsolutePath(szPath);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_PathUtils);

