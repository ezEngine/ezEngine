#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Distortion/DistortionRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
struct ezDistortionParticleData;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeDistortionFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeDistortionFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sMaskTexture;
  ezString m_sDistortionTexture;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezString m_sTintColorParameter;
  float m_fDistortionStrength = 100;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeDistortion : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeDistortion, ezParticleType);

public:
  ezParticleTypeDistortion();
  ~ezParticleTypeDistortion();

  virtual void CreateRequiredStreams() override;

  ezTexture2DResourceHandle m_hMaskTexture;
  ezTexture2DResourceHandle m_hDistortionTexture;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezTempHashedString m_sTintColorParameter;
  float m_fDistortionStrength = 100;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;

  mutable ezArrayPtr<ezDistortionParticleData> m_ParticleData;
};


