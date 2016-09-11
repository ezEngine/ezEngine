#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamSpawner.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStream.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>

class ezParticleSystemInstance;
class ezProcessingStream;
class ezParticleInitializer;

/// \brief Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer) const = 0;

  ezParticleInitializer* CreateInitializer(ezParticleSystemInstance* pOwner) const;


  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

/// \brief Base class for stream spawners that are used by ezParticleEmitter's
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer : public ezParticleModule<ezProcessingStreamSpawner, true>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer, ezProcessingStreamSpawner);

  friend class ezParticleSystemInstance;
  friend class ezParticleInitializerFactory;

protected:
  ezParticleInitializer() {}

};
