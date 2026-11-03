#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

/// Initializer that sets random particle rotation speeds
///
/// Optionally also sets a random starting rotation angle.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomRotationSpeed final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomRotationSpeed, ezParticleInitializer);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

  bool m_bRandomStartAngle = false;
  ezVarianceTypeAngle m_RotationSpeed;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    bool m_bPositiveSign;
    ezProcessingStream* m_pStreamRotationSpeed;
    ezProcessingStream* m_pStreamRotationOffset;
  };
};
