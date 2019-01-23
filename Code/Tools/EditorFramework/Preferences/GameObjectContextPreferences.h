#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class ezGameObjectContextPreferencesUser : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectContextPreferencesUser, ezPreferences);

public:
  ezGameObjectContextPreferencesUser();

  ezUuid GetContextDocument() const;
  void SetContextDocument(ezUuid val);
  ezUuid GetContextObject() const;
  void SetContextObject(ezUuid val);

protected:
  ezUuid m_ContextDocument;
  ezUuid m_ContextObject;
};
