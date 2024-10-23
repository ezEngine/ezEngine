#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringBuilder.h>

const char* ezPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (ezStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    ezUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szPathStart).AssertSuccess();

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool ezPathUtils::HasAnyExtension(ezStringView sPath)
{
  return !GetFileExtension(sPath, true).IsEmpty();
}

bool ezPathUtils::HasExtension(ezStringView sPath, ezStringView sExtension)
{
  sPath = GetFileNameAndExtension(sPath);
  ezStringView fullExt = GetFileExtension(sPath, true);

  if (sExtension.IsEmpty() && fullExt.IsEmpty())
    return true;

  // if there is a single dot at the start of the extension, remove it
  if (sExtension.StartsWith("."))
    sExtension.ChopAwayFirstCharacterAscii();

  if (!fullExt.EndsWith_NoCase(sExtension))
    return false;

  // remove the checked extension
  sPath = ezStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - sExtension.GetElementCount());

  // checked extension didn't start with a dot -> make sure there is one at the end of sPath
  if (!sPath.EndsWith("."))
    return false;

  // now make sure the rest isn't just the dot
  return sPath.GetElementCount() > 1;
}

ezStringView ezPathUtils::GetFileExtension(ezStringView sPath, bool bFullExtension)
{
  // get rid of any path before the filename
  sPath = GetFileNameAndExtension(sPath);

  // ignore all dots that the file name may start with (".", "..", ".file", "..file", etc)
  // filename may be empty afterwards, which means no dot will be found -> no extension
  while (sPath.StartsWith("."))
    sPath.ChopAwayFirstCharacterAscii();

  const char* szDot;

  if (bFullExtension)
  {
    szDot = sPath.FindSubString(".");
  }
  else
  {
    szDot = sPath.FindLastSubString(".");
  }

  // no dot at all -> no extension
  if (szDot == nullptr)
    return ezStringView();

  // dot at the very end of the string -> not an extension
  if (szDot + 1 == sPath.GetEndPointer())
    return ezStringView();

  return ezStringView(szDot + 1, sPath.GetEndPointer());
}

ezStringView ezPathUtils::GetFileNameAndExtension(ezStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return ezStringView(szSeparator + 1, sPath.GetEndPointer());
}

ezStringView ezPathUtils::GetFileName(ezStringView sPath, bool bRemoveFullExtension)
{
  // reduce the problem to just the filename + extension
  sPath = GetFileNameAndExtension(sPath);

  return GetWithoutExtension(sPath, bRemoveFullExtension);
}

ezStringView ezPathUtils::GetWithoutExtension(ezStringView sPath, bool bRemoveFullExtension)
{
  // TODO: unit test

  ezStringView ext = GetFileExtension(sPath, bRemoveFullExtension);

  if (ext.IsEmpty())
    return sPath;

  return ezStringView(sPath.GetStartPointer(), sPath.GetEndPointer() - ext.GetElementCount() - 1);
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

const char ezPathUtils::OsSpecificPathSeparator = EZ_PLATFORM_PATH_SEPARATOR;

bool ezPathUtils::IsAbsolutePath(ezStringView sPath)
{
  if (sPath.GetElementCount() < 1)
    return false;

  const char* szPath = sPath.GetStartPointer();

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  if (sPath.GetElementCount() < 2)
    return false;

  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#else
  return (szPath[0] == '/');
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
    ezUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd).AssertSuccess();

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  ref_sRoot = ezStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = ezStringView();
  }
  else
  {
    // skip path separator for the relative path
    ezUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();
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

bool ezPathUtils::IsSubPath(ezStringView sPrefixPath, ezStringView sFullPath0)
{
  if (sPrefixPath.IsEmpty())
  {
    if (sFullPath0.IsAbsolutePath())
      return true;

    EZ_REPORT_FAILURE("Prefixpath is empty and checked path is not absolute.");
    return false;
  }

  ezStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  ezStringBuilder sFullPath = sFullPath0;
  sFullPath.MakeCleanPath();

  if (sFullPath.StartsWith(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetData()[tmp.GetElementCount()] == '/';
  }

  return false;
}

bool ezPathUtils::IsSubPath_NoCase(ezStringView sPrefixPath, ezStringView sFullPath)
{
  ezStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.Trim("", "/");

  if (sFullPath.StartsWith_NoCase(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetStartPointer()[tmp.GetElementCount()] == '/';
  }

  return false;
}
