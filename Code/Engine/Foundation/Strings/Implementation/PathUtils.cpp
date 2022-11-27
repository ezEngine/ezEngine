#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
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

bool ezPathUtils::HasAnyExtension(ezStringView sPath)
{
  const char* szDot = ezStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return false;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  return (szSeparator < szDot);
}

bool ezPathUtils::HasExtension(ezStringView sPath, ezStringView sExtension)
{
  if (ezStringUtils::StartsWith(sExtension.GetStartPointer(), ".", sExtension.GetEndPointer()))
    return ezStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExtension.GetStartPointer(), sPath.GetEndPointer(), sExtension.GetEndPointer());

  ezStringBuilder sExt;
  sExt.Append(".", sExtension);

  return ezStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExt.GetData(), sPath.GetEndPointer());
}

ezStringView ezPathUtils::GetFileExtension(ezStringView sPath)
{
  const char* szDot = ezStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return ezStringView(nullptr);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator > szDot)
    return ezStringView(nullptr);

  return ezStringView(szDot + 1, sPath.GetEndPointer());
}

ezStringView ezPathUtils::GetFileNameAndExtension(ezStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return ezStringView(szSeparator + 1, sPath.GetEndPointer());
}

ezStringView ezPathUtils::GetFileName(ezStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  const char* szDot = ezStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", sPath.GetEndPointer());

  if (szDot < szSeparator) // includes (szDot == nullptr), szSeparator will never be nullptr here -> no extension
  {
    return ezStringView(szSeparator + 1, sPath.GetEndPointer());
  }

  if (szSeparator == nullptr)
  {
    if (szDot == nullptr) // no folder, no extension -> the entire thing is just a name
      return sPath;

    return ezStringView(sPath.GetStartPointer(), szDot); // no folder, but an extension -> remove the extension
  }

  // now: there is a separator AND an extension

  return ezStringView(szSeparator + 1, szDot);
}

ezStringView ezPathUtils::GetFileDirectory(ezStringView sPath)
{
  auto it = rbegin(sPath);

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(it.GetCharacter()))
    return sPath;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return ezStringView(nullptr);

  return ezStringView(sPath.GetStartPointer(), szSeparator + 1);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
const char ezPathUtils::OsSpecificPathSeparator = '\\';
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
const char ezPathUtils::OsSpecificPathSeparator = '/';
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
const char ezPathUtils::OsSpecificPathSeparator = '/';
#else
#  error "Unknown platform."
#endif

bool ezPathUtils::IsAbsolutePath(ezStringView sPath)
{
  if (sPath.GetElementCount() < 2)
    return false;

  const char* szPath = sPath.GetStartPointer();

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
  return (szPath[0] == '/');
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  return (szPath[0] == '/');
#else
#  error "Unknown platform."
#endif
}

bool ezPathUtils::IsRelativePath(ezStringView sPath)
{
  if (sPath.IsEmpty())
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (ezPathUtils::IsPathSeparator(*sPath.GetStartPointer()))
    return false;

  return !IsAbsolutePath(sPath) && !IsRootedPath(sPath);
}

bool ezPathUtils::IsRootedPath(ezStringView sPath)
{
  return !sPath.IsEmpty() && *sPath.GetStartPointer() == ':';
}

void ezPathUtils::GetRootedPathParts(ezStringView sPath, ezStringView& ref_sRoot, ezStringView& ref_sRelPath)
{
  ref_sRoot = ezStringView();
  ref_sRelPath = sPath;

  if (!IsRootedPath(sPath))
    return;

  const char* szStart = sPath.GetStartPointer();
  const char* szPathEnd = sPath.GetEndPointer();

  do
  {
    ezUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd);

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);

  ref_sRoot = ezStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = ezStringView();
  }
  else
  {
    // skip path separator for the relative path
    ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd);
    ref_sRelPath = ezStringView(szEnd, szPathEnd);
  }
}

ezStringView ezPathUtils::GetRootedPathRootName(ezStringView sPath)
{
  ezStringView root, relPath;
  GetRootedPathParts(sPath, root, relPath);
  return root;
}

bool ezPathUtils::IsValidFilenameChar(ezUInt32 uiCharacter)
{
  /// \test Not tested yet

  // Windows: https://msdn.microsoft.com/library/windows/desktop/aa365247(v=vs.85).aspx
  // Unix: https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words
  // Details can be more complicated (there might be reserved names depending on the filesystem), but in general all platforms behave like
  // this:
  static const ezUInt32 forbiddenFilenameChars[] = {'<', '>', ':', '"', '|', '?', '*', '\\', '/', '\t', '\b', '\n', '\r', '\0'};

  for (int i = 0; i < EZ_ARRAY_SIZE(forbiddenFilenameChars); ++i)
  {
    if (forbiddenFilenameChars[i] == uiCharacter)
      return false;
  }

  return true;
}

bool ezPathUtils::ContainsInvalidFilenameChars(ezStringView sPath)
{
  /// \test Not tested yet

  ezStringIterator it = sPath.GetIteratorFront();

  for (; it.IsValid(); ++it)
  {
    if (!IsValidFilenameChar(it.GetCharacter()))
      return true;
  }

  return false;
}

void ezPathUtils::MakeValidFilename(ezStringView sFilename, ezUInt32 uiReplacementCharacter, ezStringBuilder& out_sFilename)
{
  EZ_ASSERT_DEBUG(IsValidFilenameChar(uiReplacementCharacter), "Given replacement character is not allowed for filenames.");

  out_sFilename.Clear();

  for (auto it = sFilename.GetIteratorFront(); it.IsValid(); ++it)
  {
    ezUInt32 currentChar = it.GetCharacter();

    if (IsValidFilenameChar(currentChar) == false)
      out_sFilename.Append(uiReplacementCharacter);
    else
      out_sFilename.Append(currentChar);
  }
}

bool ezPathUtils::IsSubPath(ezStringView sPrefixPath, ezStringView sFullPath)
{
  /// \test this is new

  ezStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.AppendPath("");

  return sFullPath.StartsWith_NoCase(tmp);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_PathUtils);
