#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Physics/SurfaceResource.h>

class ezSurfaceAssetDocument : public ezSimpleAssetDocument<ezSurfaceResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceAssetDocument, ezSimpleAssetDocument<ezSurfaceResourceDescriptor>);

public:
  ezSurfaceAssetDocument(const char* szDocumentPath);

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
};
