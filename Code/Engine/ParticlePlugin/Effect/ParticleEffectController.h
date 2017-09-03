#pragma once

#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectController
{
public:
  ezParticleEffectController();
  ezParticleEffectController(const ezParticleEffectController& rhs);
  void operator=(const ezParticleEffectController& rhs);

  void Create(const ezParticleEffectResourceHandle& hEffectResource, ezParticleWorldModule* pModule, ezUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/);

  bool IsValid() const;
  void Invalidate();

  bool IsAlive() const;
  bool IsSharedInstance() const { return m_pSharedInstanceOwner != nullptr; }

  void SetTransform(const ezTransform& t) const;

  void Tick(const ezTime& tDiff) const;
  void SetIsInView() const;

  void StopImmediate();

  /// \brief Returns the bounding volume of the effect and the time at which the volume was updated last.
  /// The volume is in the local space of the effect.
  ezTime GetBoundingVolume(ezBoundingBoxSphere& volume) const;

private:
  ezParticleEffectController(ezParticleWorldModule* pModule, ezParticleEffectHandle hEffect);
  ezParticleEffectInstance* GetInstance() const;

  const void* m_pSharedInstanceOwner;
  ezParticleWorldModule* m_pModule;
  ezParticleEffectHandle m_hEffect;
};
