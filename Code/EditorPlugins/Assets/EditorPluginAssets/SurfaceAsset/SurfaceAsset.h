#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class ezSurfaceAssetDocument : public ezSimpleAssetDocument<ezSurfaceResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceAssetDocument, ezSimpleAssetDocument<ezSurfaceResourceDescriptor>);

public:
  ezSurfaceAssetDocument(ezStringView sDocumentPath);

protected:
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
