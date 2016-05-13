#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class ezDocument;

/// \brief Base class for all preferences.
///
/// Derive from this to implement a custom class containing preferences.
/// All properties in such a class are exposed in the preferences UI and are automatically stored and restored.
///
/// Pass the 'Domain' and 'Visibility' to the constructor to configure whether the preference class
/// is per application, per project or per document, and whether the data is shared among all users
/// or custom for every user.
class EZ_EDITORFRAMEWORK_DLL ezPreferences : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPreferences, ezReflectedClass);

public:

  enum class Domain
  {
    Application,
    Project,
    Document
  };

  enum class Visibility
  {
    Shared,
    User
  };

  /// \brief Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  template<typename TYPE>
  static TYPE* QueryPreferences(const ezDocument* pDocument = nullptr)
  {
    EZ_CHECK_AT_COMPILETIME_MSG((std::is_base_of<ezPreferences, TYPE>::value == true), "All preferences objects must be derived from ezPreferences");
    return static_cast<TYPE*>(QueryPreferences(ezGetStaticRTTI<TYPE>(), pDocument));
  }

  /// \brief Static function to query a preferences object of the given type.
  /// If the instance does not exist yet, it is created and the data is restored from file.
  static ezPreferences* QueryPreferences(const ezRTTI* pRtti, const ezDocument* pDocument = nullptr);

  /// \brief Saves all preferences that are tied to the given document
  static void SaveDocumentPreferences(const ezDocument* pDocument);

  /// \brief Removes all preferences for the given document. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearDocumentPreferences(const ezDocument* pDocument);

  /// \brief Saves all project specific preferences.
  static void SaveProjectPreferences();

  /// \brief Removes all project specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearProjectPreferences();

  /// \brief Saves all application specific preferences.
  static void SaveApplicationPreferences();

  /// \brief Removes all application specific preferences. Does not save them.
  /// Afterwards the preferences will not appear in the UI any further.
  static void ClearApplicationPreferences();

  //// \brief Fills the list with all currently known preferences
  static void GatherAllPreferences(ezHybridArray<ezPreferences*, 16>& out_AllPreferences);

  /// \brief Whether the preferences are app, project or document specific
  Domain GetDomain() const { return m_Domain; }

  /// \brief Whether the data is per user or shared
  Visibility GetVisibility() const { return m_Visibility; }

  /// \brief Within the same domain and visibility the name must be unique, but across those it can be reused.
  const ezString& GetName() const { return m_sUniqueName; }

  /// \brief If these preferences are per document, the pointer is valid, otherwise nullptr.
  const ezDocument* GetDocumentAssociation() const { return m_pDocument; }

protected:

  ezPreferences(Domain domain, Visibility visibility, const char* szUniqueName);

  ezString GetFilePath() const;

private:
  static void SavePreferences(const ezDocument* pDocument, Domain domain);
  static void ClearPreferences(const ezDocument* pDocument, Domain domain);

  void Load();
  void Save() const;

private:
  Domain m_Domain;
  Visibility m_Visibility;
  ezString m_sUniqueName;
  const ezDocument* m_pDocument;

  static ezMap<const ezDocument*, ezMap<const ezRTTI*, ezPreferences*>> s_Preferences;
};
