#include <FoundationPCH.h>

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

ezStringView ezPathUtils::GetFileExtension(const char* szPath, const char* szPathEnd)
{
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szDot = ezStringUtils::FindLastSubString(szPath, ".", nullptr, szPathEnd);

  if (szDot == nullptr)
    return ezStringView(nullptr);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  if (szSeparator > szDot)
    return ezStringView(nullptr);

  return ezStringView(szDot + 1, szPathEnd);
}

ezStringView ezPathUtils::GetFileNameAndExtension(const char* szPath, const char* szPathEnd)
{
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  if (szSeparator == nullptr)
    return ezStringView(szPath, szPathEnd);

  return ezStringView(szSeparator + 1, szPathEnd);
}

ezStringView ezPathUtils::GetFileName(const char* szPath, const char* szPathEnd)
{
  // make sure szPathEnd is valid
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  const char* szDot = ezStringUtils::FindLastSubString(szPath, ".", szPathEnd);

  if (szDot < szSeparator) // includes (szDot == nullptr), szSeparator will never be nullptr here -> no extension
  {
    return ezStringView(szSeparator + 1, szPathEnd);
  }

  if (szSeparator == nullptr)
  {
    if (szDot == nullptr) // no folder, no extension -> the entire thing is just a name
      return ezStringView(szPath, szPathEnd);

    return ezStringView(szPath, szDot); // no folder, but an extension -> remove the extension
  }

  // now: there is a separator AND an extension

  return ezStringView(szSeparator + 1, szDot);
}

ezStringView ezPathUtils::GetFileDirectory(const char* szPath, const char* szPathEnd)
{
  // make sure szPathEnd is valid
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  ezStringView end(szPath, szPathEnd);
  auto it = rbegin(end);

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(it.GetCharacter()))
    return end;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(szPath, szPathEnd);

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return ezStringView(nullptr);

  return ezStringView(szPath, szSeparator + 1);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
const char ezPathUtils::OsSpecificPathSeparator = '\\';
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
const char ezPathUtils::OsSpecificPathSeparator = '/';
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
const char ezPathUtils::OsSpecificPathSeparator = '/';
#else
#error "Unknown platform."
#endif

bool ezPathUtils::IsAbsolutePath(const char* szPath)
{
  if (ezStringUtils::IsNullOrEmpty(szPath))
    return false;

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
#error "Unknown platform."
#endif
}

bool ezPathUtils::IsRelativePath(const char* szPath)
{
  if (ezStringUtils::IsNullOrEmpty(szPath))
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (ezPathUtils::IsPathSeparator(szPath[0]))
    return false;

  return !IsAbsolutePath(szPath) && !IsRootedPath(szPath);
}

bool ezPathUtils::IsRootedPath(const char* szPath)
{
  return szPath != nullptr && szPath[0] == ':';
}

void ezPathUtils::GetRootedPathParts(const char* szPath, ezStringView& root, ezStringView& relPath)
{
  root = ezStringView();
  relPath = ezStringView(szPath);

  if (!IsRootedPath(szPath))
    return;

  const char* szStart = szPath;

  do
  {
    ezUnicodeUtils::MoveToNextUtf8(szStart);

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd);

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    ezUnicodeUtils::MoveToNextUtf8(szEnd);

  root = ezStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    relPath = ezStringView();
  }
  else
  {
    // skip path separator for the relative path
    ezUnicodeUtils::MoveToNextUtf8(szEnd);
    relPath = ezStringView(szEnd);
  }
}

ezStringView ezPathUtils::GetRootedPathRootName(const char* szPath)
{
  ezStringView root, relPath;
  GetRootedPathParts(szPath, root, relPath);
  return root;
}

bool ezPathUtils::IsValidFilenameChar(ezUInt32 character)
{
  /// \test Not tested yet

  // Windows: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
  // Unix: https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words
  // Details can be more complicated (there might be reserved names depending on the filesystem), but in general all platforms behave like
  // this:
  static const ezUInt32 forbiddenFilenameChars[] = {'<', '>', ':', '"', '|', '?', '*', '\\', '/', '\t', '\b', '\n', '\r', '\0'};

  for (int i = 0; i < EZ_ARRAY_SIZE(forbiddenFilenameChars); ++i)
  {
    if (forbiddenFilenameChars[i] == character)
      return false;
  }

  return true;
}

bool ezPathUtils::ContainsInvalidFilenameChars(const char* szPath, const char* szPathEnd /*= ezMaxStringEnd*/)
{
  /// \test Not tested yet

  // make sure szPathEnd is valid
  ezStringUtils::UpdateStringEnd(szPath, szPathEnd);

  ezStringView view(szPath, szPathEnd);
  ezStringIterator<ezStringView> it = view.GetIteratorFront();

  for (; it.IsValid(); ++it)
  {
    if (!IsValidFilenameChar(it.GetCharacter()))
      return true;
  }

  return false;
}

ezResult ezPathUtils::MakeValidFilename(const char* szFilename, ezUInt32 replacementCharacter, ezStringBuilder& outFilename)
{
  EZ_ASSERT_DEBUG(IsValidFilenameChar(replacementCharacter), "Given replacement character is not allowed for filenames.");

  // Empty string
  if (ezStringUtils::IsNullOrEmpty(szFilename))
    return EZ_FAILURE;

  do
  {
    ezUInt32 currentChar = ezUnicodeUtils::DecodeUtf8ToUtf32(szFilename); // moves pointer one char forward already.
    if (IsValidFilenameChar(currentChar) == false)
      outFilename.Append(replacementCharacter);
    else
      outFilename.Append(currentChar);

  } while (*szFilename != '\0');

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_PathUtils);

