#include <CoreUtils/PCH.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezString ezTranslationLookup::s_sSearchPath;
ezDynamicArray<ezString> ezTranslationLookup::s_TranslationFiles;
ezMap<ezUInt32, ezString> ezTranslationLookup::s_Translations;

void ezTranslationLookup::SetLanguageSearchPath(const char* szLanguageFolder)
{
  if (s_sSearchPath == szLanguageFolder)
    return;

  s_sSearchPath = szLanguageFolder;

  ReloadTranslations();
}


void ezTranslationLookup::AddTranslationFile(const char* szFileName)
{
  s_TranslationFiles.PushBack(szFileName);

  LoadTranslationFile(szFileName);
}

void ezTranslationLookup::LoadTranslationFile(const char* szFileName)
{
  EZ_LOG_BLOCK("LoadTranslationFile", szFileName);

  ezStringBuilder sPath;
  sPath.AppendPath(s_sSearchPath, szFileName);

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

    s_Translations[ezHashHelper<const char*>::Hash(sKey.GetData())] = sValue;
  }
}

const char* ezTranslationLookup::Translate(const char* szString, ezUInt32 uiStringHash)
{
  auto it = s_Translations.Find(uiStringHash);

  if (it.IsValid())
  {
    return it.Value();
  }

  ezLog::Warning("Missing Translation for: '%s'", szString);
  s_Translations[uiStringHash] = szString;

  return s_Translations[uiStringHash].GetData();
}

void ezTranslationLookup::ReloadTranslations()
{
  EZ_LOG_BLOCK("ReloadTranslations", s_sSearchPath.GetData());

  s_Translations.Clear();

  for (const auto& file : s_TranslationFiles)
  {
    LoadTranslationFile(file);
  }
}

