#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

/// Initializer that sets random particle sizes
///
/// Sizes can be picked from a variance value or sampled from a curve.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomSize final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomSize, ezParticleInitializer);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

  ezVarianceTypeFloat m_Size;
  ezCurve1DResourceHandle m_hCurve;
  ezHashedString m_sSizeScaleParameter;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamSize;
  };
};
