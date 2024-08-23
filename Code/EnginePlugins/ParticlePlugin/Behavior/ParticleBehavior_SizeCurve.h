#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_SizeCurve final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_SizeCurve, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  float m_fBaseSize;
  float m_fCurveScale;
  ezCurve1DResourceHandle m_hCurve;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_SizeCurve final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_SizeCurve, ezParticleBehavior);

public:
  float m_fBaseSize;
  float m_fCurveScale;
  ezCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezUInt8 m_uiFirstToUpdate = 0;
  ezUInt8 m_uiCurrentUpdateInterval = 8;
};
