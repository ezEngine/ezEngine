#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezSoundEventAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundEventAssetProperties, ezReflectedClass);

public:
  ezSoundEventAssetProperties() {}


};


class ezSoundEventAssetDocument : public ezSimpleAssetDocument<ezSoundEventAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSoundEventAssetDocument, ezSimpleAssetDocument<ezSoundEventAssetProperties>);

public:
  ezSoundEventAssetDocument(const char* szDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

};
