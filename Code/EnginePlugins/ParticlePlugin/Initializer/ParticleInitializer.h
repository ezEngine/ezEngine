#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleSystemInstance;
class ezProcessingStream;
class ezParticleInitializer;
class ezParticleEffectInstance;

/// \brief Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const = 0;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const;

  ezParticleInitializer* CreateInitializer(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream) = 0;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const {}
};

/// \brief Base class for stream spawners that are used by ezParticleEmitter's
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer, ezParticleModule);

  friend class ezParticleSystemInstance;
  friend class ezParticleInitializerFactory;

protected:
  ezParticleInitializer();

  virtual void Process(ezUInt64 uiNumElements) final override {}
};
