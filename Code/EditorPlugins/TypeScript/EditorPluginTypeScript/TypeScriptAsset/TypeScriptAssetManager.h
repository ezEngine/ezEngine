#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

class ezTypeScriptAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetDocumentManager, ezAssetDocumentManager);

public:
  ezTypeScriptAssetDocumentManager();
  ~ezTypeScriptAssetDocumentManager();

  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override { return "ezTypeScriptRes"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("TypeScript");
  }

  ezTypeScriptTranspiler& GetTranspiler() { return m_Transpiler; }

  ezResult GenerateScriptCompendium();

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

  void InitializeTranspiler();
  bool m_bTranspilerLoaded = false;
  ezTypeScriptTranspiler m_Transpiler;

  void SetupProjectForTypeScript();

  ezDocumentTypeDescriptor m_AssetDesc;

  ezMap<ezString, ezTimestamp> m_CheckedTsFiles;
};
