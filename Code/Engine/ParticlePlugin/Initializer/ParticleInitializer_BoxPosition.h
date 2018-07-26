#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class ezParticleInitializerFactory_BoxPosition : public ezParticleInitializerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory_BoxPosition, ezParticleInitializerFactory);

public:
  ezParticleInitializerFactory_BoxPosition();

  virtual const ezRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezVec3 m_vPositionOffset;
  ezVec3 m_vSize;
  ezString m_sScaleXParameter;
  ezString m_sScaleYParameter;
  ezString m_sScaleZParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer_BoxPosition : public ezParticleInitializer
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
