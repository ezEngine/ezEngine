#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory_Rise : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_Rise, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_Rise();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  // negative for falling down
  float m_fMinRiseSpeed;
  float m_fRiseSpeedRange;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_Rise : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_Rise, ezParticleInitializer);

public:
  virtual void AfterPropertiesConfigured() override;

  float m_fMinRiseSpeed;
  float m_fRiseSpeedRange;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  // used to rise/fall along the gravity vector
  ezPhysicsWorldModuleInterface* m_pPhysicsModule;
};

