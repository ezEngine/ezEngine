#pragma once

#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetProperties.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>


class ezCustomDataAssetDocument : public ezSimpleAssetDocument<ezCustomDataAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCustomDataAssetDocument, ezAssetDocument);

public:
  ezCustomDataAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
                                                   const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};

