#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

struct EZ_PARTICLEPLUGIN_DLL ezQuadParticleOrientation
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Billboard,

    Rotating_OrthoEmitterDir,
    Rotating_EmitterDir,

    Fixed_EmitterDir,
    Fixed_WorldUp,
    Fixed_RandomDir,

    FixedAxis_EmitterDir,
    FixedAxis_ParticleDir,

    Default = Billboard
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezQuadParticleOrientation);

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuadFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeQuadFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

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
  float m_fStretch = 1;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuad final : public ezParticleType
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
  float m_fStretch = 1;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

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
  void AddParticleRenderData(ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const;
  void CreateExtractedData(const ezHybridArray<sod, 64>* pSorted) const;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamRotationSpeed = nullptr;
  ezProcessingStream* m_pStreamRotationOffset = nullptr;
  ezProcessingStream* m_pStreamAxis = nullptr;
  ezProcessingStream* m_pStreamVariation = nullptr;
  ezProcessingStream* m_pStreamLastPosition = nullptr;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
  mutable ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
};
