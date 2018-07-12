#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/Basics.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;

struct EZ_PARTICLEPLUGIN_DLL ezQuadParticleOrientation
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Billboard,

    FragmentOrthogonalEmitterDirection,
    FragmentEmitterDirection,
    // these would require additional streams
    // ParticleDirection, // -> last position for direction
    // FragmentRandom, // -> rotation axis

    SpriteEmitterDirection,
    SpriteWorldUp,
    SpriteRandom,

    Default = Billboard
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezQuadParticleOrientation);

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuadFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeQuadFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezEnum<ezQuadParticleOrientation> m_Orientation;
  ezAngle m_MaxDeviation;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezString m_sTexture;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezString m_sTintColorParameter;
  ezString m_sDistortionTexture;
  float m_fDistortionStrength = 0;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuad : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeQuad, ezParticleType);

public:
  ezParticleTypeQuad();
  ~ezParticleTypeQuad();

  virtual void CreateRequiredStreams() override;

  ezEnum<ezQuadParticleOrientation> m_Orientation;
  ezAngle m_MaxDeviation;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezTempHashedString m_sTintColorParameter;
  ezTexture2DResourceHandle m_hDistortionTexture;
  float m_fDistortionStrength = 0;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform,
                                     ezUInt64 uiExtractedFrame) const override;

  struct sod
  {
    EZ_DECLARE_POD_TYPE();

    float dist;
    ezUInt32 index;
  };


protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}
  void AllocateParticleData(const ezUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const;
  void AddParticleRenderData(ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform) const;
  void CreateExtractedData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform,
                           ezUInt64 uiExtractedFrame, const ezHybridArray<sod, 64>* pSorted) const;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
  ezProcessingStream* m_pStreamAxis = nullptr;
  ezProcessingStream* m_pStreamVariation = nullptr;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  mutable ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
};
