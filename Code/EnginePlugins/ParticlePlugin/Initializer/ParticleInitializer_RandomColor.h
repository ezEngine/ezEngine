#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;

/// Initializer that sets random particle colors
///
/// Colors are picked randomly between Color1 and Color2, or sampled from a gradient.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomColor final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomColor, ezParticleInitializer);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

  ezColor m_Color1;
  ezColor m_Color2;
  ezEnum<ezGradientSource> m_GradientSource;
  ezColorGradient m_Gradient;
  ezColorGradientResourceHandle m_hSharedGradient;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamColor;
  };
};
