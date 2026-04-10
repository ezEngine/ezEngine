#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezColorGradientResourceHandle = ezTypedResourceHandle<class ezColorGradientResource>;

/// Initializer that sets random particle colors
///
/// Colors are picked randomly between Color1 and Color2, or sampled from a gradient.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_RandomColor final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomColor, ezParticleInitializerFactory);

public:
  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezColor m_Color1;
  ezColor m_Color2;
  ezEnum<ezGradientSource> m_GradientSource;
  ezColorGradient m_Gradient;
  ezColorGradientResourceHandle m_hSharedGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomColor final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomColor, ezParticleInitializer);

public:
  ezColor m_Color1;
  ezColor m_Color2;
  const ezColorGradient* m_pGradient = nullptr;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamColor = nullptr;
};
