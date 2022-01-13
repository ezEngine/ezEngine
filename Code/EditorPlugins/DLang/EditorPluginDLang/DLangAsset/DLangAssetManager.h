#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Status.h>

struct ezGameObjectDocumentEvent;

class ezDLangAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangAssetDocumentManager, ezAssetDocumentManager);

public:
  ezDLangAssetDocumentManager();
  ~ezDLangAssetDocumentManager();

  //void SetupProjectForDLang(bool bForce);
  //ezResult GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags);

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  void GameObjectDocumentEventHandler(const ezGameObjectDocumentEvent& e);

  bool m_bProjectSetUp = false;

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;

  //ezMap<ezString, ezTimestamp> m_CheckedTsFiles;
};

//////////////////////////////////////////////////////////////////////////

class ezDLangPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangPreferences, ezPreferences);

public:
  ezDLangPreferences();

  //bool m_bAutoUpdateScriptsForSimulation = true;
  //bool m_bAutoUpdateScriptsForPlayTheGame = true;
};
