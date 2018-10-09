#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameEngine/Surfaces/SurfaceResource.h>

class ezSurfaceAssetDocument : public ezSimpleAssetDocument<ezSurfaceResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceAssetDocument, ezSimpleAssetDocument<ezSurfaceResourceDescriptor>);

public:
  ezSurfaceAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Surface"; }

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
};
