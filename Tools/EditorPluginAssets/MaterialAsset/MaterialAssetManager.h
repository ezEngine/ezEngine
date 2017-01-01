#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezMaterialAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMaterialAssetDocumentManager, ezAssetDocumentManager);

public:
  ezMaterialAssetDocumentManager();
  ~ezMaterialAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezMaterialBin"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Material");
  }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;
  virtual ezString GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const char* szPlatform) const override;
  virtual bool IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, ezUInt16 uiTypeVersion) override;

  static const char* const s_szShaderOutputTag;
private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const;

  virtual bool GeneratesPlatformSpecificAssets() const override { return false; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
};

