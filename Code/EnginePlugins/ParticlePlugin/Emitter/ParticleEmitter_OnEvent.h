#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

/// Emitter that spawns particles in response to events
///
/// Only spawns when the specified event is raised.
/// Useful for creating impact effects or other event-driven particles.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_OnEvent final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_OnEvent, ezParticleEmitter);

public:
  ezParticleEmitter_OnEvent();
  ~ezParticleEmitter_OnEvent();

  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezParticleEmitterState IsFinished(const ezParticleModuleContext& ctx) const override;
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& diff) const override;
  virtual void ProcessEventQueue(const ezParticleModuleContext& ctx, ezParticleEventQueue queue) const override;

  ezHashedString m_sEventName;          ///< Name of the event that triggers emission
  ezUInt32 m_uiSpawnCountMin = 1;       ///< Minimum particles per event
  ezUInt32 m_uiSpawnCountRange = 0;     ///< Random range added to spawn count
  ezHashedString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    bool m_bSpawn;
  };
};
