#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>

class ezParticleSystemInstance;
class ezStream;
class ezParticleInitializer;

/// \brief Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory, ezReflectedClass);

public:
  ezParticleInitializerFactory();

  virtual const ezRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const = 0;

  ezParticleInitializer* CreateInitializer(ezParticleSystemInstance* pOwner) const;


  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

/// \brief Base class for stream spawners that are used by ezParticleEmitter's
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer : public ezStreamElementSpawner
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer, ezStreamElementSpawner);

  friend class ezParticleSystemInstance;
  friend class ezParticleInitializerFactory;

public:
  /// \brief Called after construction when m_pOwnerSystem is set and when properties have been set
  virtual void AfterPropertiesConfigured() {}

protected:
  ezParticleInitializer();

  virtual ezResult UpdateStreamBindings() override;

  ezParticleSystemInstance* m_pOwnerSystem;
  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
  ezStream* m_pStreamColor;
  ezStream* m_pStreamLifeTime;
};
