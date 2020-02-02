#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>
#include <EditorFramework/Preferences/Preferences.h>

struct ezGameObjectDocumentEvent;

class ezTypeScriptAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetDocumentManager, ezAssetDocumentManager);

public:
  ezTypeScriptAssetDocumentManager();
  ~ezTypeScriptAssetDocumentManager();

  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override { return "ezTypeScriptRes"; }

  ezTypeScriptTranspiler& GetTranspiler() { return m_Transpiler; }

  void SetupProjectForTypeScript(bool bForce);
  ezResult GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags);

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;

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

  ezDocumentTypeDescriptor m_AssetDesc;

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
