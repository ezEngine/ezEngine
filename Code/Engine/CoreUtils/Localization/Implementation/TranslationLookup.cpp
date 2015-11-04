#include <CoreUtils/PCH.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezDynamicArray<ezString> ezTranslatorFromFiles::s_TranslationFiles;
ezHybridArray<ezUniquePtr<ezTranslator>, 16> ezTranslationLookup::s_pTranslators;

void ezTranslationLookup::AddTranslator(ezUniquePtr<ezTranslator> pTranslator)
{
  s_pTranslators.PushBack(std::move(pTranslator));
}


const char* ezTranslationLookup::Translate(const char* szString, ezUInt32 uiStringHash)
{
  for (ezUInt32 i = s_pTranslators.GetCount(); i > 0; --i)
  {
    const char* szResult = s_pTranslators[i - 1]->Translate(szString, uiStringHash);

    if (szResult != nullptr)
      return szResult;
  }

  return nullptr;
}


void ezTranslationLookup::Clear()
{
  s_pTranslators.Clear();
}

const char* ezTranslatorFromFiles::Translate(const char* szString, ezUInt32 uiStringHash)
{
  if (m_uiLoadedFiles != s_TranslationFiles.GetCount())
    ReloadTranslations();

  return ezTranslatorStorage::Translate(szString, uiStringHash);
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

  ezLog::Dev("Loading Localization File '%s'", sPath.GetData());

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    /// \todo fall back to different language

    ezLog::SeriousWarning("Failed to open localization file '%s'", sPath.GetData());
    return;
  }

  ezStringBuilder sContent;
  sContent.ReadAll(file);

  ezDeque<ezString> Lines;
  sContent.Split(false, Lines, "\n");

  ezStringBuilder sLine, sKey, sValue;
  for (const auto& line : Lines)
  {
    sLine = line;

    const char* szSeperator = sLine.FindSubString(";");

    if (szSeperator == nullptr)
    {
      ezLog::Error("Invalid line in translation file: '%s'", sLine.GetData());
      continue;
    }

    sKey.SetSubString_FromTo(sLine.GetData(), szSeperator);
    sValue = szSeperator + 1; // the rest

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");

    StoreTranslation(sValue, ezHashHelper<const char*>::Hash(sKey.GetData()));
  }
}

void ezTranslatorStorage::StoreTranslation(const char* szString, ezUInt32 uiStringHash)
{
  m_Translations[uiStringHash] = szString;
}

const char* ezTranslatorStorage::Translate(const char* szString, ezUInt32 uiStringHash)
{
  auto it = m_Translations.Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return nullptr;
}

void ezTranslatorStorage::Reset()
{
  m_Translations.Clear();
}

const char* ezTranslatorLogMissing::Translate(const char* szString, ezUInt32 uiStringHash)
{
  const char* szResult = ezTranslatorStorage::Translate(szString, uiStringHash);

  if (szResult != nullptr)
    return szResult;

  ezLog::Warning("Missing Translation for '%s'", szString);

  StoreTranslation(szString, uiStringHash);
  return szString;
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Localization_Implementation_TranslationLookup);

