#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_FadeOut : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_FadeOut, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_FadeOut : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_FadeOut, ezParticleBehavior);

public:
  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezUInt8 m_uiFirstToUpdate = 0;
  ezUInt8 m_uiCurrentUpdateInterval = 2;
};
