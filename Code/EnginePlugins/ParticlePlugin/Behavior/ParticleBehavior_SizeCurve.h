#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that modifies particle size over their lifetime using a curve
///
/// The curve is sampled based on the particle's normalized lifetime (0-1).
/// The final size is: base size + (curve value * curve scale).
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_SizeCurve final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_SizeCurve, ezParticleBehavior);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  ezEnum<ezCurveSource> m_CurveSource;
  ezSingleCurveData m_Curve;
  ezCurve1DResourceHandle m_hSharedCurve;
  float m_fSizeCurveOffset = 0;
  float m_fSizeCurveScale = 1;
  mutable ezCurve1D m_RuntimeCurve;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime = nullptr;
    ezProcessingStream* m_pStreamSize = nullptr;
    ezUInt8 m_uiFirstToUpdate = 0;
    ezUInt8 m_uiCurrentUpdateInterval = 8;
  };
};
