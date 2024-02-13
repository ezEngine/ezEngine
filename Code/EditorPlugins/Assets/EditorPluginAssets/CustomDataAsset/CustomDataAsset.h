#pragma once

#include <Core/Utils/CustomData.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezCustomDataAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetProperties, ezReflectedClass);

public:
  ezCustomData* m_pType = nullptr;
};


class ezCustomDataAssetDocument : public ezSimpleAssetDocument<ezCustomDataAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetDocument, ezSimpleAssetDocument<ezCustomDataAssetProperties>);

public:
  ezCustomDataAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
};
