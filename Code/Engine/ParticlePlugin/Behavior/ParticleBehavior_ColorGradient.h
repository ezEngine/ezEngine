#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <GameUtils/Curves/ColorGradientResource.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_ColorGradient : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_ColorGradient, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetColorGradient(const ezColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  EZ_FORCE_INLINE const ezColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

private:
  ezColorGradientResourceHandle m_hGradient;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_ColorGradient : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_ColorGradient, ezParticleBehavior);

public:
  ezColorGradientResourceHandle m_hGradient;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezStream* m_pStreamLifeTime;
  ezStream* m_pStreamColor;
};
