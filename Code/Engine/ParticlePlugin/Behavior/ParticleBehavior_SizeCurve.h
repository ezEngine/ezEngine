#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <GameUtils/Curves/Curve1DResource.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_SizeCurve : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_SizeCurve, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  void SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  float m_fMinSize;
  float m_fSizeRange;
  ezCurve1DResourceHandle m_hCurve;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_SizeCurve : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_SizeCurve, ezParticleBehavior);

public:
  float m_fMinSize;
  float m_fSizeRange;
  ezCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezStream* m_pStreamLifeTime;
  ezStream* m_pStreamSize;
};
