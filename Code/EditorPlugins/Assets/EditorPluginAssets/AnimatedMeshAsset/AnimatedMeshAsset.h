#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class ezMeshResourceDescriptor;
class ezMaterialAssetDocument;

class ezAnimatedMeshAssetDocument : public ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimatedMeshAssetDocument, ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>);

public:
  ezAnimatedMeshAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;

  ezStatus CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc);


  virtual ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
