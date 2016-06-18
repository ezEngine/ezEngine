#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameUtils/Collection/CollectionResource.h>

class ezCollectionAssetEntry : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollectionAssetEntry, ezReflectedClass);
public:

  ezString m_sLookupName;
  ezString m_sRedirectionAsset;
};

class ezCollectionAssetData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollectionAssetData, ezReflectedClass);
public:

  ezDynamicArray<ezCollectionAssetEntry> m_Entries;
};



class ezCollectionAssetDocument : public ezSimpleAssetDocument<ezCollectionAssetData>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCollectionAssetDocument, ezSimpleAssetDocument<ezCollectionAssetData>);

public:
  ezCollectionAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Collection Asset"; }

  virtual const char* QueryAssetType() const override { return "Collection"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform) override;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override { return ezStatus(EZ_SUCCESS); }
};
