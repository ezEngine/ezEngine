#include <CoreUtils/PCH.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezDynamicArray<ezString> ezTranslatorFromFiles::s_TranslationFiles;
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

const char* ezTranslatorFromFiles::Translate(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  if (m_uiLoadedFiles != s_TranslationFiles.GetCount())
    ReloadTranslations();

  return ezTranslatorStorage::Translate(szString, uiStringHash, usage);
}


void ezTranslatorFromFiles::SetSearchPath(const char* szFolder)
{
  m_sSearchPath = szFolder;
  m_uiLoadedFiles = 0;
}


void ezTranslatorFromFiles::AddTranslationFile(const char* szFileName)
{
  s_TranslationFiles.PushBack(szFileName);
}


void ezTranslatorFromFiles::Reset()
{
  ezTranslatorStorage::Reset();
  m_uiLoadedFiles = 0;
}

void ezTranslatorFromFiles::ReloadTranslations()
{
  EZ_LOG_BLOCK("ReloadTranslations", m_sSearchPath.GetData());

  Reset();

  for (const auto& file : s_TranslationFiles)
  {
    LoadTranslationFile(file);
  }

  m_uiLoadedFiles = s_TranslationFiles.GetCount();
}

void ezTranslatorFromFiles::LoadTranslationFile(const char* szFileName)
{
  EZ_LOG_BLOCK("LoadTranslationFile", szFileName);

  ezStringBuilder sPath;
  sPath.AppendPath(m_sSearchPath, szFileName);

  ezLog::Dev("Loading Localization File '{0}'", sPath.GetData());

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Failed to open localization file '{0}'", sPath.GetData());
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
      ezLog::Error("Invalid line in translation file: '{0}'", sLine.GetData());
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

void ezTranslatorStorage::StoreTranslation(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  m_Translations[usage][uiStringHash] = szString;
}

const char* ezTranslatorStorage::Translate(const char* szString, ezUInt32 uiStringHash, ezTranslationUsage usage)
{
  auto it = m_Translations[usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return nullptr;
}

void ezTranslatorStorage::Reset()
{
  for (ezUInt32 i = 0; i < ezTranslationUsage::ENUM_COUNT; ++i)
  {
    m_Translations[i].Clear();
  }
}


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



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Localization_Implementation_TranslationLookup);

