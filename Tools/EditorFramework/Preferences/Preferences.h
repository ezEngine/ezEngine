#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class ezDocument;

class EZ_EDITORFRAMEWORK_DLL ezPreferences : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPreferences, ezReflectedClass);

public:

  template<typename TYPE>
  static TYPE* GetPreferences(const ezDocument* pDocument = nullptr)
  {
    return static_cast<TYPE*>(GetPreferences(ezGetStaticRTTI<TYPE>(), pDocument));
  }

  static ezPreferences* GetPreferences(const ezRTTI* pRtti, const ezDocument* pDocument = nullptr);

  static void SaveDocumentPreferences(const ezDocument* pDocument);

  static void SaveProjectAndEditorPreferences();

protected:
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

  ezPreferences(Domain domain, Visibility visibility, const char* szUniqueName);

  ezString GetFilePath() const;

private:
  void Load();
  void Save() const;

private:
  Domain m_Domain;
  Visibility m_Visibility;
  ezString m_sUniqueName;
  const ezDocument* m_pDocument;

  static ezMap<const ezDocument*, ezMap<const ezRTTI*, ezPreferences*>> s_Preferences;
};
