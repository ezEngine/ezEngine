#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Sprite/SpriteRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
struct ezSpriteParticleData;

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

  ezString m_sTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeSprite : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeSprite, ezParticleType);

public:
  virtual void CreateRequiredStreams() override;

  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezSpriteAxis> m_RotationAxis;
  ezAngle m_MaxDeviation;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamRotationSpeed;
  ezProcessingStream* m_pStreamAxis;

  mutable ezSharedPtr<ezSpriteParticleDataContainer> m_GpuData;
};


