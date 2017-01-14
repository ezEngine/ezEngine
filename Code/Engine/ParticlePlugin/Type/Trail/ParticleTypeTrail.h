#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/Basics.h>
#include <Foundation/Types/SharedPtr.h>
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

  ezUInt16 m_uiMaxSegments;
  ezTime m_UpdateDiff;
  ezString m_sTexture;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeTrail : public ezParticleType
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeTrail, ezParticleType);

public:
  ezParticleTypeTrail();
  ~ezParticleTypeTrail();

  virtual void CreateRequiredStreams() override;

  ezUInt16 m_uiMaxSegments;
  ezTime m_UpdateDiff;
  ezTexture2DResourceHandle m_hTexture;

  virtual void ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const override;

protected:
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);
  ezUInt32 GetMaxSegmentBucketSize() const;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamSize;
  ezProcessingStream* m_pStreamColor;
  ezProcessingStream* m_pStreamTrailData;
  ezTime m_LastSnapshot;
  ezUInt8 m_uiCurFirstIndex;
  ezUInt8 m_uiCurLastIndex;
  ezUInt8 m_uiMaxSegmentsMask;

  mutable ezSharedPtr<ezTrailParticleDataContainer> m_GpuData;
  mutable ezSharedPtr<ezTrailParticleSegmentDataContainer> m_SegmentGpuData;

  struct TrailData8
  {
    ezVec3 m_Positions[8];
  };

  struct TrailData16
  {
    ezVec3 m_Positions[16];
  };

  struct TrailData32
  {
    ezVec3 m_Positions[32];
  };

  struct TrailData64
  {
    ezVec3 m_Positions[64];
  };

  struct TrailData
  {
    ezUInt16 m_uiNumSegments;
    ezUInt16 m_uiTrailDataIndex;
  };

  ezUInt16 GetNewTrailDataIndex();
  const ezVec3* GetTrailDataPositions(ezUInt32 index) const;
  ezVec3* GetTrailDataPositions(ezUInt32 index);

  /// \todo Use a shared freelist across effects instead
  ezDynamicArray<TrailData8> m_TrailData8;
  ezDynamicArray<TrailData16> m_TrailData16;
  ezDynamicArray<TrailData32> m_TrailData32;
  ezDynamicArray<TrailData64> m_TrailData64;
  ezDynamicArray<ezUInt16> m_FreeTrailData; // 16 Bit ought to be enough for anybody
};


