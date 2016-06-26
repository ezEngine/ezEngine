#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GameUtils/Surfaces/SurfaceResource.h>

class ezSurfaceAssetDocument : public ezSimpleAssetDocument<ezSurfaceResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceAssetDocument, ezSimpleAssetDocument<ezSurfaceResourceDescriptor>);

public:
  ezSurfaceAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Surface Asset"; }

  virtual const char* QueryAssetType() const override { return "Surface"; }

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override { return ezStatus(EZ_SUCCESS); }
};
