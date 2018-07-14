#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class ezParticleInitializerFactory_SpherePosition : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_SpherePosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_SpherePosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  ezVarianceTypeFloat m_Speed;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_SpherePosition : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_SpherePosition, ezParticleInitializer);

public:
  ezVec3 m_vPositionOffset;
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  ezVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};
