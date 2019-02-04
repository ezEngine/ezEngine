#include <PCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

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

//////////////////////////////////////////////////////////////////////////

ezHybridArray<ezUniquePtr<ezTranslator>, 16> ezTranslationLookup::s_pTranslators;

void ezTranslationLookup::AddTranslator(ezUniquePtr<ezTranslator> pTranslator)
{
  s_pTranslators.PushBack(std::move(pTranslator));
}


const char* ezTranslationLookup::Translate(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  for (ezUInt32 i = s_pTranslators.GetCount(); i > 0; --i)
  {
    const char* szResult = s_pTranslators[i - 1]->Translate(szString, uiStringHash, usage);

    if (szResult != nullptr)
      return szResult;
  }

  return nullptr;
}


void ezTranslationLookup::Clear()
{
  s_pTranslators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void ezTranslatorFromFiles::LoadTranslationFilesFromFolder(const char* szFolder)
{
  EZ_LOG_BLOCK("LoadTranslationFilesFromFolder", szFolder);

  if (m_sFolder != szFolder)
  {
    // prevent assigning to itself during Reload()
    m_sFolder = szFolder;
  }

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  ezStringBuilder startPath;
  if (ezFileSystem::ResolvePath(szFolder, &startPath, nullptr).Failed())
    return;

  ezStringBuilder fullpath;

  ezFileSystemIterator it;
  if (it.StartSearch(startPath, true, false).Succeeded())
  {
    do
    {
      fullpath = it.GetCurrentPath();
      fullpath.AppendPath(it.GetStats().m_sFileName);

      LoadTranslationFile(fullpath);
    } while (it.Next().Succeeded());
  }
#endif
}

void ezTranslatorFromFiles::Reload()
{
  ezTranslatorStorage::Reload();

  if (!m_sFolder.IsEmpty())
  {
    LoadTranslationFilesFromFolder(m_sFolder);
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

  ezDeque<ezString> Lines;
  sContent.Split(false, Lines, "\n");

  ezStringBuilder sLine, sKey, sValue, sTooltip;
  for (const auto& line : Lines)
  {
    sLine = line;
    sLine.Trim(" \t\r\n");

    if (sLine.IsEmpty())
      continue;

    const char* szSeperator = sLine.FindSubString(";");

    if (szSeperator == nullptr)
    {
      ezLog::Error("Invalid line in translation file: '{0}'", sLine);
      continue;
    }

    sKey.SetSubString_FromTo(sLine.GetData(), szSeperator);

    const char* szSeperator2 = sLine.FindSubString(";", szSeperator + 1);

    if (szSeperator2 == nullptr)
    {
      sValue = szSeperator + 1; // the rest
      sTooltip.Clear();
    }
    else
    {
      sValue.SetSubString_FromTo(szSeperator + 1, szSeperator2);
      sTooltip = szSeperator2 + 1;
    }

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");

    StoreTranslation(sValue, ezHashHelper<const char*>::Hash(sKey.GetData()), ezTranslationUsage::Default);
    StoreTranslation(sTooltip, ezHashHelper<const char*>::Hash(sKey.GetData()), ezTranslationUsage::Tooltip);
  }
}

//////////////////////////////////////////////////////////////////////////

void ezTranslatorStorage::StoreTranslation(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  m_Translations[(ezUInt32)usage][uiStringHash] = szString;
}

const char* ezTranslatorStorage::Translate(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  auto it = m_Translations[(ezUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return nullptr;
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

const char* ezTranslatorLogMissing::Translate(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  const char* szResult = ezTranslatorStorage::Translate(szString, uiStringHash, usage);

  if (szResult != nullptr)
    return szResult;

  if (usage == ezTranslationUsage::Tooltip)
    return "";

  if (ezTranslatorLogMissing::s_bActive)
  {
    ezLog::Warning("Missing Translation for '{0}'", szString);
  }

  StoreTranslation(szString, uiStringHash, usage);
  return szString;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_TranslationLookup);

