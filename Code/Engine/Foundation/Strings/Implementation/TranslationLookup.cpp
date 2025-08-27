#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

bool ezTranslator::s_bHighlightUntranslated = false;
ezHybridArray<ezTranslator*, 4> ezTranslator::s_AllTranslators;

ezTranslator::ezTranslator()
{
  s_AllTranslators.PushBack(this);
}

ezTranslator::~ezTranslator()
{
  s_AllTranslators.RemoveAndSwap(this);
}

void ezTranslator::Reset() {}

void ezTranslator::Reload() {}

void ezTranslator::ReloadAllTranslators()
{
  EZ_LOG_BLOCK("ReloadAllTranslators");

  for (ezTranslator* pTranslator : s_AllTranslators)
  {
    pTranslator->Reload();
  }
}

void ezTranslator::HighlightUntranslated(bool bHighlight)
{
  if (s_bHighlightUntranslated == bHighlight)
    return;

  s_bHighlightUntranslated = bHighlight;

  ReloadAllTranslators();
}

//////////////////////////////////////////////////////////////////////////

ezHybridArray<ezUniquePtr<ezTranslator>, 16> ezTranslationLookup::s_Translators;

void ezTranslationLookup::AddTranslator(ezUniquePtr<ezTranslator> pTranslator)
{
  s_Translators.PushBack(std::move(pTranslator));
}


ezStringView ezTranslationLookup::Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  for (ezUInt32 i = s_Translators.GetCount(); i > 0; --i)
  {
    ezStringView sResult = s_Translators[i - 1]->Translate(sString, uiStringHash, usage);

    if (!sResult.IsEmpty())
      return sResult;
  }

  if (usage != ezTranslationUsage::Default)
    return {};

  return sString;
}


void ezTranslationLookup::Clear()
{
  s_Translators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void ezTranslatorFromFiles::AddTranslationFilesFromFolder(const char* szFolder)
{
  EZ_LOG_BLOCK("AddTranslationFilesFromFolder", szFolder);

  if (!m_Folders.Contains(szFolder))
  {
    m_Folders.PushBack(szFolder);
  }

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezStringBuilder startPath;
  if (ezFileSystem::ResolvePath(szFolder, &startPath, nullptr).Failed())
    return;

  ezStringBuilder fullpath;

  ezFileSystemIterator it;
  it.StartSearch(startPath, ezFileSystemIteratorFlags::ReportFilesRecursive);


  while (it.IsValid())
  {
    fullpath = it.GetCurrentPath();
    fullpath.AppendPath(it.GetStats().m_sName);

    LoadTranslationFile(fullpath);

    it.Next();
  }

#endif
}

ezStringView ezTranslatorFromFiles::Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  return ezTranslatorStorage::Translate(sString, uiStringHash, usage);
}

void ezTranslatorFromFiles::Reload()
{
  ezTranslatorStorage::Reload();

  for (const auto& sFolder : m_Folders)
  {
    AddTranslationFilesFromFolder(sFolder);
  }
}

void ezTranslatorFromFiles::LoadTranslationFile(const char* szFullPath)
{
  EZ_LOG_BLOCK("LoadTranslationFile", szFullPath);

  ezLog::Dev("Loading Localization File '{0}'", szFullPath);

  ezFileReader file;
  if (file.Open(szFullPath).Failed())
  {
    ezLog::Warning("Failed to open localization file '{0}'", szFullPath);
    return;
  }

  ezStringBuilder sContent;
  sContent.ReadAll(file);

  ezDeque<ezStringView> Lines;
  sContent.Split(false, Lines, "\n");

  ezHybridArray<ezStringView, 4> entries;

  ezStringBuilder sLine, sKey, sValue, sTooltip, sHelpUrl;
  for (const auto& line : Lines)
  {
    sLine = line;
    sLine.Trim(" \t\r\n");

    if (sLine.IsEmpty() || sLine.StartsWith("#"))
      continue;

    entries.Clear();
    sLine.Split(true, entries, ";");

    if (entries.GetCount() <= 1)
    {
      ezLog::Error("Invalid line in translation file: '{0}'", sLine);
      continue;
    }

    sKey = entries[0];
    sValue = entries[1];

    sTooltip.Clear();
    sHelpUrl.Clear();

    if (entries.GetCount() >= 3)
      sTooltip = entries[2];
    if (entries.GetCount() >= 4)
      sHelpUrl = entries[3];

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");
    sTooltip.Trim(" \t\r\n");
    sHelpUrl.Trim(" \t\r\n");

    if (GetHighlightUntranslated())
    {
      sValue.Prepend("# ");
      sValue.Append(" (@", sKey, ")");
    }

    StoreTranslation(sValue, ezHashingUtils::StringHash(sKey), ezTranslationUsage::Default);
    StoreTranslation(sTooltip, ezHashingUtils::StringHash(sKey), ezTranslationUsage::Tooltip);
    StoreTranslation(sHelpUrl, ezHashingUtils::StringHash(sKey), ezTranslationUsage::HelpURL);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezTranslatorStorage::StoreTranslation(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  m_Translations[(ezUInt32)usage][uiStringHash] = sString;
}

ezStringView ezTranslatorStorage::Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  EZ_IGNORE_UNUSED(sString);

  auto it = m_Translations[(ezUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return {};
}

void ezTranslatorStorage::Reset()
{
  for (ezUInt32 i = 0; i < (ezUInt32)ezTranslationUsage::ENUM_COUNT; ++i)
  {
    m_Translations[i].Clear();
  }
}

void ezTranslatorStorage::Reload()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////

bool ezTranslatorLogMissing::s_bActive = true;

ezStringView ezTranslatorLogMissing::Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  if (!ezTranslatorLogMissing::s_bActive && !GetHighlightUntranslated())
    return {};

  if (usage != ezTranslationUsage::Default)
    return {};

  ezStringView sResult = ezTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (sResult.IsEmpty())
  {
    ezLog::Warning("Missing translation: {0};", sString);

    StoreTranslation(sString, uiStringHash, usage);
  }

  return {};
}

ezStringView ezTranslatorMakeMoreReadable::Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage)
{
  if (usage != ezTranslationUsage::Default)
    return {};

  ezStringView sResult = ezTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (!sResult.IsEmpty())
    return sResult;

  ezStringBuilder result;
  ezStringBuilder tmp = sString;
  tmp.Trim(" _-");

  if (tmp.TrimWordStart("ez"))
  {
    ezStringView sComponent = "Component";
    if (tmp.EndsWith(sComponent) && tmp.GetElementCount() > sComponent.GetElementCount())
    {
      tmp.Shrink(0, sComponent.GetElementCount());
    }
  }

  auto IsUpper = [](ezUInt32 c)
  { return c == ezStringUtils::ToUpperChar(c); };
  auto IsNumber = [](ezUInt32 c)
  { return c >= '0' && c <= '9'; };

  ezUInt32 uiPrev = ' ';
  ezUInt32 uiCur = ' ';
  ezUInt32 uiNext = ' ';

  bool bContinue = true;

  for (auto it = tmp.GetIteratorFront(); bContinue; ++it)
  {
    uiPrev = uiCur;
    uiCur = uiNext;

    if (it.IsValid())
    {
      uiNext = it.GetCharacter();
    }
    else
    {
      uiNext = ' ';
      bContinue = false;
    }

    if (uiCur == '_')
      uiCur = ' ';

    if (uiCur == ':')
    {
      result.Clear();
      continue;
    }

    if (uiPrev != '[' && uiCur != ']' && IsNumber(uiPrev) != IsNumber(uiCur))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (IsNumber(uiPrev) && IsNumber(uiCur))
    {
      result.Append(uiCur);
      continue;
    }

    if (IsUpper(uiPrev) && IsUpper(uiCur) && !IsUpper(uiNext))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (!IsUpper(uiCur) && IsUpper(uiNext))
    {
      result.Append(uiCur);
      result.Append(" ");
      continue;
    }

    result.Append(uiCur);
  }

  result.Trim(" ");
  while (result.ReplaceAll("  ", " ") > 0)
  {
    // remove double whitespaces
  }

  if (GetHighlightUntranslated())
  {
    result.Append(" (@", sString, ")");
  }

  StoreTranslation(result, uiStringHash, usage);

  return ezTranslatorStorage::Translate(sString, uiStringHash, usage);
}
