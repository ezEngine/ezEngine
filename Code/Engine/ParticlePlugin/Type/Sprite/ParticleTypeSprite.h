#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Sprite/SpriteRenderer.h>

typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;
struct ezSpriteParticleData;

struct EZ_PARTICLEPLUGIN_DLL ezSpriteAxis
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    EmitterX,
    EmitterY,
    EmitterZ,
    WorldX,
    WorldY,
    WorldZ,
    Random,

    Default = EmitterZ
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

  ezString m_sTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;
  ezAngle m_SpinPerSecond;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeSprite : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeSprite, ezParticleType);

public:
  ezParticleTypeSprite();

  virtual void CreateRequiredStreams() override;

  ezTextureResourceHandle m_hTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;
  ezAngle m_SpinPerSecond;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamRotationSpeed;

  mutable ezUInt64 m_uiLastExtractedFrame;
  mutable ezSharedPtr<ezSpriteParticleDataContainer> m_GpuData;
};


