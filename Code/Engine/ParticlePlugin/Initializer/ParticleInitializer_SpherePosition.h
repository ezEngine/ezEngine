#pragma once

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
  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  float m_fMinSpeed;
  float m_fSpeedRange;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_SpherePosition : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_SpherePosition, ezParticleInitializer);

public:

  float m_fRadius;
  bool m_bSpawnOnSurface;
  bool m_bSetVelocity;
  float m_fMinSpeed;
  float m_fSpeedRange;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

};
