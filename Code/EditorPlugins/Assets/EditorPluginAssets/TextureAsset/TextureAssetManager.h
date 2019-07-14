#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <GameEngine/Configuration/PlatformProfile.h>
#include <Foundation/Types/Status.h>

class ezTextureAssetProfileConfig : public ezProfileConfigData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetProfileConfig, ezProfileConfigData);

public:
  ezUInt16 m_uiMaxResolution = 1024 * 16;
};

class ezTextureAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureAssetDocumentManager, ezAssetDocumentManager);

public:
  ezTextureAssetDocumentManager();
  ~ezTextureAssetDocumentManager();


  virtual ezString GetResourceTypeExtension(const char* szDocumentPath) const override;

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Texture 2D");
    inout_AssetTypeNames.Insert("Render Target");
  }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezUInt64 ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const override;

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                          ezDocument*& out_pDocument) override;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
  ezDocumentTypeDescriptor m_AssetDescRT;
};
