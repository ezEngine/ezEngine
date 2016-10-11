#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class ezParticleInitializerFactory_Life : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_Life, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_Life();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezTime m_MinLifeTime;
  ezTime m_LifeTimeRange;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_Life : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_Life, ezParticleInitializer);

public:

  ezTime m_MinLifeTime;
  ezTime m_LifeTimeRange;


  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime;
};
