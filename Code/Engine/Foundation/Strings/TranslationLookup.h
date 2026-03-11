#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief What a translated string is used for.
enum class ezTranslationUsage
{
  Default,
  Tooltip,
  HelpURL,

  ENUM_COUNT
};

/// \brief Base class to translate one string into another
class EZ_FOUNDATION_DLL ezTranslator
{
public:
  ezTranslator();
  virtual ~ezTranslator();

  /// \brief The given string (with the given hash) shall be translated
  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) = 0;

  /// \brief Called to reset internal state
  virtual void Reset();

  /// \brief May reload the known translations
  virtual void Reload();

  /// \brief Will call Reload() on all currently active translators
  static void ReloadAllTranslators();

  static void HighlightUntranslated(bool bHighlight);

  static bool GetHighlightUntranslated() { return s_bHighlightUntranslated; }

private:
  static bool s_bHighlightUntranslated;
  static ezHybridArray<ezTranslator*, 4> s_AllTranslators;
};

/// \brief Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class EZ_FOUNDATION_DLL ezTranslatorPassThrough : public ezTranslator
{
public:
  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) override
  {
    EZ_IGNORE_UNUSED(uiStringHash);
    EZ_IGNORE_UNUSED(usage);
    return sString;
  }
};

/// \brief Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is
/// not known
class EZ_FOUNDATION_DLL ezTranslatorStorage : public ezTranslator
{
public:
  /// \brief Stores szString as the translation for the string with the given hash
  virtual void StoreTranslation(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage);

  /// \brief Returns the translated string for uiStringHash, or nullptr, if not available
  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) override;

  /// \brief Clears all stored translation strings
  virtual void Reset() override;

  /// \brief Simply executes Reset() on this translator
  virtual void Reload() override;

protected:
  ezMap<ezUInt64, ezString> m_Translations[(int)ezTranslationUsage::ENUM_COUNT];
};

/// \brief Outputs a 'Missing Translation' warning the first time a string translation is requested.
/// Otherwise always returns nullptr, allowing the next translator to take over.
class EZ_FOUNDATION_DLL ezTranslatorLogMissing : public ezTranslatorStorage
{
public:
  /// Can be used from external code to (temporarily) deactivate error logging (a bit hacky)
  static bool s_bActive;

  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) override;
};

/// \brief Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them.
class EZ_FOUNDATION_DLL ezTranslatorFromFiles : public ezTranslatorStorage
{
public:
  /// \brief Loads all files recursively from the specified folder as translation files.
  ///
  /// The given path must be absolute or resolvable to an absolute path.
  /// On failure, the function does nothing.
  /// This function depends on ezFileSystemIterator to be available.
  void AddTranslationFilesFromFolder(const char* szFolder);

  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) override;

  virtual void Reload() override;

private:
  void LoadTranslationFile(const char* szFullPath);

  ezHybridArray<ezString, 4> m_Folders;
};

/// \brief Returns the same string that is passed into it, but strips off class names and separates the text at CamelCase boundaries.
class EZ_FOUNDATION_DLL ezTranslatorMakeMoreReadable : public ezTranslatorStorage
{
public:
  virtual ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage) override;
};

/// \brief Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class EZ_FOUNDATION_DLL ezTranslationLookup
{
public:
  /// \brief Translators will be queried in the reverse order that they were added.
  static void AddTranslator(ezUniquePtr<ezTranslator> pTranslator);

  /// \brief Prefer to use the ezTranslate macro instead of calling this function directly. Will query all translators for a translation,
  /// until one is found.
  static ezStringView Translate(ezStringView sString, ezUInt64 uiStringHash, ezTranslationUsage usage);

  /// \brief Deletes all translators.
  static void Clear();

private:
  static ezHybridArray<ezUniquePtr<ezTranslator>, 16> s_Translators;
};

/// \brief Use this macro to query a translation for a string from the ezTranslationLookup system
#define ezTranslate(string) ezTranslationLookup::Translate(string, ezHashingUtils::StringHash(string), ezTranslationUsage::Default)

/// \brief Use this macro to query a translation for a tooltip string from the ezTranslationLookup system
#define ezTranslateTooltip(string) ezTranslationLookup::Translate(string, ezHashingUtils::StringHash(string), ezTranslationUsage::Tooltip)

/// \brief Use this macro to query a translation for a help URL from the ezTranslationLookup system
#define ezTranslateHelpURL(string) ezTranslationLookup::Translate(string, ezHashingUtils::StringHash(string), ezTranslationUsage::HelpURL)
