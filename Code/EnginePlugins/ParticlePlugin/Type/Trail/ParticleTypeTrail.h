#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
struct ezTrailParticleData;

/// Factory for creating trail particle types.
class EZ_PARTICLEPLUGIN_DLL ezParticleTypeTrailFactory final : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeTrailFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezUInt16 m_uiMaxPoints;
  ezTime m_UpdateDiff;
  ezString m_sTexture;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezString m_sTintColorParameter;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  bool m_bUseCustomMaterial = false;
  ezString m_sCustomMaterial;
};

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
  ezParticleTypeTrail();
  ~ezParticleTypeTrail();

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezUInt16 m_uiMaxPoints;
  ezTime m_UpdateDiff;
  ezTexture2DResourceHandle m_hTexture;
  ezEnum<ezParticleTextureAtlasType> m_TextureAtlasType;
  ezUInt8 m_uiNumSpritesX = 1;
  ezUInt8 m_uiNumSpritesY = 1;
  ezTempHashedString m_sTintColorParameter;
  ezEnum<ezParticleLightingMode> m_LightingMode;
  float m_fNormalCurvature = 0.5f;
  float m_fLightDirectionality = 0.5f;
  ezMaterialResourceHandle m_hCustomMaterial;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const override;

  /// Returns an approximation of the maximum trail radius.
  ///
  /// This is an estimate based on particle size and maximum trail length.
  /// Inspecting actual trail positions would be more accurate but costly.
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return fParticleSize + m_uiMaxPoints * 0.05f; }

  /// Computes the memory bucket size needed for storing trail points.
  static ezUInt16 ComputeTrailPointBucketSize(ezUInt16 uiMaxTrailPoints);

protected:
  friend class ezParticleTypeTrailFactory;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamTrailData = nullptr;
  ezProcessingStream* m_pStreamVariation = nullptr;
  ezTime m_LastSnapshot;
  ezUInt8 m_uiCurFirstIndex = 0;
  float m_fSnapshotFraction;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezTrailParticleShaderData> m_TrailParticleData;
  mutable ezArrayPtr<ezVec4> m_TrailPointsShared;

  struct TrailData
  {
    ezUInt16 m_uiNumPoints;
    ezUInt16 m_uiIndexForTrailPoints;
  };

  ezUInt16 GetIndexForTrailPoints();
  const ezVec4* GetTrailPointsPositions(ezUInt32 index) const;
  ezVec4* GetTrailPointsPositions(ezUInt32 index);

  // Currently only 64-point trails are used. Smaller bucket sizes are reserved for future use.
  ezDynamicArray<ezTrailParticlePointsData64, ezAlignedAllocatorWrapper> m_TrailPoints64; ///< Storage for trail points
  ezDynamicArray<ezUInt16> m_FreeTrailData;                                               ///< Freelist for trail data allocation
};
