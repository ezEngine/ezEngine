#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

struct ezGameObjectDocumentEvent;

class ezTypeScriptAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetDocumentManager, ezAssetDocumentManager);

public:
  ezTypeScriptAssetDocumentManager();
  ~ezTypeScriptAssetDocumentManager();

  ezTypeScriptTranspiler& GetTranspiler() { return m_Transpiler; }

  void SetupProjectForTypeScript(bool bForce);
  ezResult GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags);

  virtual ezStatus GetAdditionalOutputs(ezDynamicArray<ezString>& files) override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  void ToolsProjectEventHandler(const ezToolsProjectEvent& e);
  void GameObjectDocumentEventHandler(const ezGameObjectDocumentEvent& e);

  static void ModifyTsBeforeTranspilation(ezStringBuilder& source);

  void InitializeTranspiler();
  void ShutdownTranspiler();

  bool m_bTranspilerLoaded = false;
  bool m_bProjectSetUp = false;
  ezTypeScriptTranspiler m_Transpiler;

  ezAssetDocumentTypeDescriptor m_DocTypeDesc;

  ezMap<ezString, ezTimestamp> m_CheckedTsFiles;
};

//////////////////////////////////////////////////////////////////////////

class ezTypeScriptPreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptPreferences, ezPreferences);

public:
  ezTypeScriptPreferences();

  bool m_bAutoUpdateScriptsForSimulation = true;
  bool m_bAutoUpdateScriptsForPlayTheGame = true;
};
