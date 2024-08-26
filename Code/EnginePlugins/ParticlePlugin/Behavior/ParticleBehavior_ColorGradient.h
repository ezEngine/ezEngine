#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_ColorGradient final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_ColorGradient, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  // ************************************* PROPERTIES ***********************************

  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor = ezColor::White;
  ezColorGradientResourceHandle m_hGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_ColorGradient final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_ColorGradient, ezParticleBehavior);

public:
  ezColorGradientResourceHandle m_hGradient;
  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class ezParticleBehaviorFactory_ColorGradient;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamColor = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
  ezColor m_InitColor;
  ezUInt8 m_uiFirstToUpdate = 0;
  ezUInt8 m_uiCurrentUpdateInterval = 8;
};
