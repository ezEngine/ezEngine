#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

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

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
