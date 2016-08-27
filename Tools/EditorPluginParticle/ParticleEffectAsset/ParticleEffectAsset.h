#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <Foundation/Communication/Event.h>

struct ezParticleEffectAssetEvent
{
  enum Type
  {
    RestartEffect,
  };

  Type m_Type;
};

class ezParticleEffectAssetDocument : public ezSimpleAssetDocument<ezParticleEffectDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectAssetDocument, ezSimpleAssetDocument<ezParticleEffectDescriptor>);

public:
  ezParticleEffectAssetDocument(const char* szDocumentPath);

  virtual const char* QueryAssetType() const override { return "Particle Effect"; }

  ezStatus WriteParticleEffectAsset(ezStreamWriter& stream, const char* szPlatform) const;

  void TriggerRestartEffect();

  ezEvent<const ezParticleEffectAssetEvent&> m_Events;

protected:
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override { return ezStatus(EZ_SUCCESS); }

};
