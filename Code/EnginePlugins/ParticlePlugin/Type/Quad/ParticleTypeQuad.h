#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Quad/QuadParticleRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;

/// Orientation modes for quad particles.
struct EZ_PARTICLEPLUGIN_DLL ezQuadParticleOrientation
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Billboard,                ///< Always faces the camera

    Rotating_OrthoEmitterDir, ///< Rotates around axis orthogonal to emitter direction
    Rotating_EmitterDir,      ///< Rotates around emitter direction

    Fixed_EmitterDir,         ///< Fixed orientation based on emitter direction
    Fixed_WorldUp,            ///< Fixed orientation aligned with world up
    Fixed_RandomDir,          ///< Fixed random orientation per particle

    FixedAxis_EmitterDir,     ///< Fixed axis aligned with emitter direction
    FixedAxis_ParticleDir,    ///< Fixed axis aligned with particle direction

    Default = Billboard
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezQuadParticleOrientation);

/// Factory for creating quad particle types.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuadFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeQuadFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  ezHashedString m_sTexture;
  ezHashedString m_sCustomMaterial;
  ezHashedString m_sTintColorParameter;

  ezEnum<ezQuadParticleOrientation> m_Orientation;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  bool m_bUseCustomMaterial = false;

  ezAngle m_MaxDeviation;
  float m_fStretch = 1;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;
};

/// Renders particles as camera-facing or oriented textured quads.
///
/// This is the most common particle type, rendering each particle as a textured
/// quad. Quads can be billboarded to face the camera or oriented in various ways.
/// Supports texture atlases for sprite animations and variations. Can be lit or
/// fullbright. The stretch parameter allows elongating particles along their
/// velocity direction for motion blur effects.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeQuad final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeQuad, ezParticleType);

public:
  ezParticleTypeQuad();
  ~ezParticleTypeQuad();

  virtual void CreateRequiredStreams() override;

  ezTexture2DResourceHandle m_hTexture;
  ezMaterialResourceHandle m_hCustomMaterial;
  ezTempHashedString m_sTintColorParameter;

  ezEnum<ezQuadParticleOrientation> m_Orientation;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;

  ezAngle m_MaxDeviation;
  float m_fStretch = 1;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Helper struct for depth sorting particles.
  struct sod
  {
    EZ_DECLARE_POD_TYPE();

    float dist;     ///< Distance from camera
    ezUInt32 index; ///< Particle index
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
