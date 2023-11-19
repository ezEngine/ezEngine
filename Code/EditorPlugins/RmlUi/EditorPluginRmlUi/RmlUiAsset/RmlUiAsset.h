#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct ezRmlUiResourceDescriptor;

class ezRmlUiAssetDocument : public ezSimpleAssetDocument<ezRmlUiAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAssetDocument, ezSimpleAssetDocument<ezRmlUiAssetProperties>);

public:
  ezRmlUiAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
