#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Fragment/FragmentRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct EZ_PARTICLEPLUGIN_DLL ezFragmentAxis
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    OrthogonalEmitterDirection,
    EmitterDirection,
    // these would require additional streams
    //ParticleDirection, // -> last position for direction
    //Random, // -> rotation axis

    Default = OrthogonalEmitterDirection
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezFragmentAxis);

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeFragmentFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeFragmentFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sTexture;
  ezEnum<ezFragmentAxis> m_RotationAxis;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeFragment : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeFragment, ezParticleType);

public:
  virtual void CreateRequiredStreams() override;

  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezFragmentAxis> m_RotationAxis;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezTangentQuadParticleShaderData> m_QuadParticleData;
};


