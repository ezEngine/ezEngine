#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <GameEngine/Curves/ColorGradientResource.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_ColorGradient : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_ColorGradient, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetColorGradient(const ezColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  EZ_ALWAYS_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  ezEnum<ezParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  ezColor m_TintColor = ezColor::White;

private:
  ezColorGradientResourceHandle m_hGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_ColorGradient : public ezParticleBehavior
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
