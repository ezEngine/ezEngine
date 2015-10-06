#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

class EZ_COREUTILS_DLL ezTranslationLookup
{
public:
  static void SetLanguageSearchPath(const char* szLanguageFolder);
  static void AddTranslationFile(const char* szFileName);

  /// \brief Prefer to use the ezTranslate instead of calling this function directly
  static const char* Translate(const char* szString, ezUInt32 uiStringHash);

private:
  static void ReloadTranslations();
  static void LoadTranslationFile(const char* szFileName);

  static ezString s_sSearchPath;
  static ezDynamicArray<ezString> s_TranslationFiles;
  static ezMap<ezUInt32, ezString> s_Translations;
};

#define ezTranslate(string) ezTranslationLookup::Translate(string, ezHashHelper<const char*>::Hash(string))
