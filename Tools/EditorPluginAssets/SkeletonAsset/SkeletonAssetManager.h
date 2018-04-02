#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class ezSkeletonAssetDocumentManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonAssetDocumentManager, ezAssetDocumentManager);

public:
  ezSkeletonAssetDocumentManager();
  ~ezSkeletonAssetDocumentManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezSkeleton"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("Skeleton");
  }

  virtual ezBitflags<ezAssetDocumentFlags> GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const override;

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const;

  virtual bool GeneratesPlatformSpecificAssets() const override { return false; }

private:
  ezDocumentTypeDescriptor m_AssetDesc;
};

