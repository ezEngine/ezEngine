#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

class ezParticleEffectAssetDocument : public ezSimpleAssetDocument<ezParticleEffectDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectAssetDocument, ezSimpleAssetDocument<ezParticleEffectDescriptor>);

public:
  ezParticleEffectAssetDocument(const char* szDocumentPath);

  virtual const char* GetDocumentTypeDisplayString() const override { return "Particle Effect Asset"; }

  virtual const char* QueryAssetType() const override;

  ezStatus WriteParticleEffectAsset(ezStreamWriter& stream, const char* szPlatform) const;

protected:
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;


  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

};
