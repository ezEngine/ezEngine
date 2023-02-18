#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Bounds final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Bounds, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Bounds();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezVec3 m_vPositionOffset;
  ezVec3 m_vBoxExtents;
  ezEnum<ezParticleOutOfBoundsMode> m_OutOfBoundsMode;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Bounds final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Bounds, ezParticleBehavior);

public:
  ezVec3 m_vPositionOffset;
  ezVec3 m_vBoxExtents;
  ezEnum<ezParticleOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamLastPosition = nullptr;
};
