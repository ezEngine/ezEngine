#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/Trail/TrailRenderer.h>

typedef ezTypedResourceHandle<class ezTexture2DResource> ezTexture2DResourceHandle;
struct ezTrailParticleData;

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeTrailFactory : public ezParticleTypeFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeTrailFactory, ezParticleTypeFactory);

public:
  virtual const ezRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(ezParticleType* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezUInt16 m_uiMaxPoints;
  ezTime m_UpdateDiff;
  ezString m_sTexture;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeTrail : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeTrail, ezParticleType);

public:
  ezParticleTypeTrail();
  ~ezParticleTypeTrail();


  ezEnum<ezParticleTypeRenderMode> m_RenderMode;
  ezUInt16 m_uiMaxPoints;
  ezTime m_UpdateDiff;
  ezTexture2DResourceHandle m_hTexture;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;
  /// \todo This is a hacky guess, one would actually need to inspect the trail positions
  virtual float GetMaxParticleRadius(float fParticleSize) const override { return fParticleSize + m_uiMaxPoints * 0.05f; }

  static ezUInt32 ComputeTrailPointBucketSize(ezUInt32 uiMaxTrailPoints);

protected:
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamTrailData = nullptr;
  ezTime m_LastSnapshot;
  ezUInt8 m_uiCurFirstIndex;
  float m_fSnapshotFraction;

  mutable ezArrayPtr<ezBaseParticleShaderData> m_BaseParticleData;
  mutable ezArrayPtr<ezVec4> m_TrailPointsShared;

  struct TrailData
  {
    ezUInt16 m_uiNumPoints;
    ezUInt16 m_uiIndexForTrailPoints;
  };

  ezUInt16 GetIndexForTrailPoints();
  const ezVec4* GetTrailPointsPositions(ezUInt32 index) const;
  ezVec4* GetTrailPointsPositions(ezUInt32 index);

  /// \todo Use a shared freelist across effects instead
  //ezDynamicArray<ezTrailParticlePointsData8> m_TrailPoints8;
  //ezDynamicArray<ezTrailParticlePointsData16> m_TrailPoints16;
  //ezDynamicArray<ezTrailParticlePointsData32> m_TrailPoints32;
  ezDynamicArray<ezTrailParticlePointsData64, ezAlignedAllocatorWrapper> m_TrailPoints64;
  ezDynamicArray<ezUInt16> m_FreeTrailData;
};


