#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that modifies particle size over their lifetime using a curve
///
/// The curve is sampled based on the particle's normalized lifetime (0-1).
/// The final size is: base size + (curve value * curve scale).
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_SizeCurve final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_SizeCurve, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  ezEnum<ezCurveSource> m_CurveSource;
  ezSingleCurveData m_Curve;
  ezCurve1DResourceHandle m_hSharedCurve;
  float m_fSizeCurveOffset = 0;
  float m_fSizeCurveScale = 1;
  mutable ezCurve1D m_RuntimeCurve;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_SizeCurve final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_SizeCurve, ezParticleBehavior);

public:
  const ezCurve1D* m_pCurve = nullptr;
  float m_fSizeCurveOffset = 0;
  float m_fSizeCurveScale = 1;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamSize = nullptr;
  ezUInt8 m_uiFirstToUpdate = 0;
  ezUInt8 m_uiCurrentUpdateInterval = 8;
};
