#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
struct ezTrailParticleData;

/// Renders particles as textured ribbons following their movement path.
///
/// Trails record particle positions over time and render a ribbon connecting
/// the trail points. The trail length is determined by the maximum number of
/// points and the update interval. Older trail points fade out automatically.
/// Trails can be lit or fullbright and support texture atlases.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeTrail final : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeTrail, ezParticleType);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool GetReceiveParticleDeath() const override { return true; }
  virtual void OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;
  virtual void ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Returns an approximation of the maximum trail radius.
  ///
  /// This is an estimate based on particle size and maximum trail length.
  /// Inspecting actual trail positions would be more accurate but costly.
  virtual float GetMaxParticleRadius(const ezParticleModuleContext& ctx, float fParticleSize) const override { return fParticleSize + ctx.GetInstanceData<InstanceData>().m_uiMaxPoints * 0.05f; }

  /// Computes the memory bucket size needed for storing trail points.
  static ezUInt16 ComputeTrailPointBucketSize(ezUInt16 uiMaxTrailPoints);

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezUInt16 m_uiMaxPoints;
  ezTime m_UpdateDiff;
  ezHashedString m_sTexture;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezEnum<ezParticleTextureAtlasOrientation> m_TextureAtlasOrientation;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezHashedString m_sTintColorParameter;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  bool m_bUseCustomMaterial = false;
  ezHashedString m_sCustomMaterial;

  ezTexture2DResourceHandle m_hTexture;
  ezMaterialResourceHandle m_hCustomMaterial;

private:
  struct TrailData
  {
    ezUInt16 m_uiNumPoints;
    ezUInt16 m_uiIndexForTrailPoints;
  };

  struct InstanceData
  {
    ezUInt16 GetIndexForTrailPoints();
    const ezVec4* GetTrailPointsPositions(ezUInt32 uiIndex) const;
    ezVec4* GetTrailPointsPositions(ezUInt32 uiIndex);

    ezProcessingStream* m_pStreamLifeTime = nullptr;
    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamSize = nullptr;
    ezProcessingStream* m_pStreamColor = nullptr;
    ezProcessingStream* m_pStreamTrailData = nullptr;
    ezProcessingStream* m_pStreamVariation = nullptr;
    ezUInt16 m_uiMaxPoints = 0; ///< Clamped to what the bucket size allows.
    bool m_bInitialized = false;
    ezTime m_LastSnapshot;
    ezUInt8 m_uiCurFirstIndex = 0;
    float m_fSnapshotFraction = 0.0f;

    ezUInt64 m_uiLastExtractedFrame = 0;
    ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
    ezArrayPtr<ezTrailParticleShaderData> m_TrailParticleData;
    ezArrayPtr<ezVec4> m_TrailPointsShared;

    // Currently only 64-point trails are used. Smaller bucket sizes are reserved for future use.
    ezDynamicArray<ezTrailParticlePointsData64, ezAlignedAllocatorWrapper> m_TrailPoints64; ///< Storage for trail points
    ezDynamicArray<ezUInt16> m_FreeTrailData;                                               ///< Freelist for trail data allocation
  };
};
