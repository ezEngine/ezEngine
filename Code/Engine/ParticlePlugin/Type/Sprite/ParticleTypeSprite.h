#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Sprite/SpriteRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct EZ_PARTICLEPLUGIN_DLL ezSpriteAxis
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    EmitterDirection,
    WorldUp,
    Random,

    Default = EmitterDirection
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezSpriteAxis);

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeSpriteFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeSpriteFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezString m_sTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezString m_sTintColorParameter;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeSprite : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeSprite, ezParticleType);

public:
  virtual void CreateRequiredStreams() override;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezTempHashedString m_sTintColorParameter;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
  ezProcessingStream* m_pStreamAxis = nullptr;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezTangentQuadParticleShaderData> m_QuadParticleData;
};


