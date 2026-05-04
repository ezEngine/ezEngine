#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

/// Initializer that sets random particle sizes
///
/// Sizes can be picked from a variance value or sampled from a curve.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_RandomSize final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_RandomSize, ezParticleInitializerFactory);

public:
  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezVarianceTypeFloat m_Size;
  ezCurve1DResourceHandle m_hCurve;
  ezHashedString m_sSizeScaleParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_RandomSize final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_RandomSize, ezParticleInitializer);

public:
  ezVarianceTypeFloat m_Size;
  ezCurve1DResourceHandle m_hCurve;
  ezTempHashedString m_sSizeScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamSize = nullptr;
};
