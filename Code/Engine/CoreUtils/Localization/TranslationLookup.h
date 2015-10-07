#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief Base class to translate one string into another
class EZ_COREUTILS_DLL ezTranslator
{
public:
  ezTranslator() {}
  virtual ~ezTranslator() {}

  /// \brief The given string (with the given hash) shall be translated
  virtual const char* Translate(const char* szString, ezUInt32 uiStringHash) = 0;

  /// \brief Called to reset internal state
  virtual void Reset() {}
};

/// \brief Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class EZ_COREUTILS_DLL ezTranslatorPassThrough : public ezTranslator
{
public:

  virtual const char* Translate(const char* szString, ezUInt32 uiStringHash) override 
  {
    return szString;
  }
};

/// \brief Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is not known
class EZ_COREUTILS_DLL ezTranslatorStorage : public ezTranslator
{
public:
  /// \brief Stores szString as the translation for the string with the given hash
  virtual void StoreTranslation(const char* szString, ezUInt32 uiStringHash);

  /// \brief Returns the translated string for uiStringHash, or nullptr, if not available
  virtual const char* Translate(const char* szString, ezUInt32 uiStringHash) override;

  /// \brief Clears all stored translation strings
  virtual void Reset() override;

protected:
  ezMap<ezUInt32, ezString> m_Translations;
};

/// \brief Outputs a 'Missing Translation' warning the first time a string translation is requested. Otherwise returns the input string as the translation.
class EZ_COREUTILS_DLL ezTranslatorLogMissing : public ezTranslatorStorage
{
public:

  virtual const char* Translate(const char* szString, ezUInt32 uiStringHash) override;
};

/// \brief Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them
class EZ_COREUTILS_DLL ezTranslatorFromFiles : public ezTranslatorStorage
{
public:

  virtual const char* Translate(const char* szString, ezUInt32 uiStringHash) override;

  /// \brief Sets the basic search path for the translation files.
  void SetSearchPath(const char* szFolder);

  /// \brief Adds a sub-file to load from the search path. All ezTranslatorFromFiles instances will reload their translations when necessary.
  static void AddTranslationFile(const char* szFileName);

  /// \brief Resets all internal state except the search paths. Ensures translations will be reloaded.
  virtual void Reset() override;

protected:
  void ReloadTranslations();
  void LoadTranslationFile(const char* szFileName);

  ezString m_sSearchPath;

  ezUInt32 m_uiLoadedFiles;
  static ezDynamicArray<ezString> s_TranslationFiles;
};

/// \brief Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class EZ_COREUTILS_DLL ezTranslationLookup
{
public:
  /// \brief Translators will be queried in the reverse order that they were added.
  static void AddTranslator(ezUniquePtr<ezTranslator> pTranslator);

  /// \brief Prefer to use the ezTranslate instead of calling this function directly. Will query all translators for a translation, until one is found.
  static const char* Translate(const char* szString, ezUInt32 uiStringHash);

  /// \brief Deletes all translators.
  static void Clear();

private:
  static void ReloadTranslations();
  static void LoadTranslationFile(const char* szFileName);

  static ezHybridArray<ezUniquePtr<ezTranslator>, 16> s_pTranslators;
};

/// \brief Use this macro to query a translation for a string from the ezTranslationLookup system
#define ezTranslate(string) ezTranslationLookup::Translate(string, ezHashHelper<const char*>::Hash(string))
