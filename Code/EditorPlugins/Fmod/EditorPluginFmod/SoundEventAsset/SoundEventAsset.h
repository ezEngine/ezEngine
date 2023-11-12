#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezSoundEventAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundEventAssetProperties, ezReflectedClass);

public:
  ezSoundEventAssetProperties() = default;
};


class ezSoundEventAssetDocument : public ezSimpleAssetDocument<ezSoundEventAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundEventAssetDocument, ezSimpleAssetDocument<ezSoundEventAssetProperties>);

public:
  ezSoundEventAssetDocument(ezStringView sDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
