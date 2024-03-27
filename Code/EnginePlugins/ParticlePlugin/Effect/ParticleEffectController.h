#pragma once

#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectController
{
public:
  ezParticleEffectController();
  ezParticleEffectController(const ezParticleEffectController& rhs);
  void operator=(const ezParticleEffectController& rhs);

  void Create(const ezParticleEffectResourceHandle& hEffectResource, ezParticleWorldModule* pModule, ezUInt64 uiRandomSeed,
    const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/, ezArrayPtr<ezParticleEffectFloatParam> floatParams,
    ezArrayPtr<ezParticleEffectColorParam> colorParams);

  bool IsValid() const;
  void Invalidate();

  bool IsAlive() const;
  bool IsSharedInstance() const { return m_pSharedInstanceOwner != nullptr; }

  bool IsContinuousEffect() const { return GetInstance()->IsContinuous(); }

  void SetTransform(const ezTransform& t, const ezVec3& vParticleStartVelocity) const;

  void Tick(const ezTime& diff) const;

  void ExtractRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& systemTransform) const;

  void StopImmediate();

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(ezBoundingBoxSphere& ref_volume) const;

  void UpdateWindSamples(ezTime tDiff);

  /// \brief Ensures that the effect is considered to be 'visible', which affects the update rate.
  void ForceVisible();

  ezUInt64 GetNumActiveParticles() const;

  /// \name Effect Parameters
  ///@{
public:
  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const ezTempHashedString& sName, float value);

  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const ezTempHashedString& sName, const ezColor& value);

  ///@}

private:
  friend class ezParticleWorldModule;

  ezParticleEffectController(ezParticleWorldModule* pModule, ezParticleEffectHandle hEffect);
  ezParticleEffectInstance* GetInstance() const;

  const void* m_pSharedInstanceOwner = nullptr;
  ezParticleWorldModule* m_pModule = nullptr;
  ezParticleEffectHandle m_hEffect;
};
