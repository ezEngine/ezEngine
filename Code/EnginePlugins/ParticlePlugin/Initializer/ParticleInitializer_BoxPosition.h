#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

/// Initializer that spawns particles in a box volume
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_BoxPosition final : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_BoxPosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_BoxPosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  ezVec3 m_vPositionOffset;    ///< Center of the spawn box
  ezVec3 m_vSize;              ///< Size of the spawn box (full size, not half extents)
  ezString m_sScaleXParameter; ///< Optional parameter name to scale box width
  ezString m_sScaleYParameter; ///< Optional parameter name to scale box depth
  ezString m_sScaleZParameter; ///< Optional parameter name to scale box height
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_BoxPosition final : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_BoxPosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  ezVec3 m_vSize;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;
};
