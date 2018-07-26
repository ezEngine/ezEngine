#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class ezParticleInitializerFactory_VelocityCone : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_VelocityCone, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_VelocityCone();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const override;

public:
  ezAngle m_Angle;
  ezVarianceTypeFloat m_Speed;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_VelocityCone : public ezParticleInitializer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer_VelocityCone, ezParticleInitializer);

public:
  ezAngle m_Angle;
  ezVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamVelocity;
};
