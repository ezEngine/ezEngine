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

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Helper struct for depth sorting particles.
  struct sod
  {
    EZ_DECLARE_POD_TYPE();

    float dist;     ///< Distance from camera
    ezUInt32 index; ///< Particle index
  };

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  ezHashedString m_sTexture;
  ezHashedString m_sCustomMaterial;
  ezHashedString m_sTintColorParameter;

  ezTexture2DResourceHandle m_hTexture;
  ezMaterialResourceHandle m_hCustomMaterial;

  ezEnum<ezQuadParticleOrientation> m_Orientation;
  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezEnum<ezParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  bool m_bUseCustomMaterial = false;

  ezAngle m_MaxDeviation;
  float m_fStretch = 1;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  float m_fGeometryProximityFadeOut = 0.1f;
  float m_fCameraProximityFadeOut = 0.5f;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime;
    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamSize;
    ezProcessingStream* m_pStreamColor;
    ezProcessingStream* m_pStreamRotationSpeed;
    ezProcessingStream* m_pStreamRotationOffset;
    ezProcessingStream* m_pStreamAxis;
    ezProcessingStream* m_pStreamVariation;
    ezProcessingStream* m_pStreamLastPosition;

    ezUInt64 m_uiLastExtractedFrame;
    ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
    ezArrayPtr<ezBillboardQuadParticleShaderData> m_BillboardParticleData;
    ezArrayPtr<ezTangentQuadParticleShaderData> m_TangentParticleData;
  };

  static void AllocateParticleData(InstanceData& data, const ezUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData);
  void AddParticleRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const;
  void CreateExtractedData(const ezParticleModuleContext& ctx, const ezHybridArray<sod, 64>* pSorted) const;
};
