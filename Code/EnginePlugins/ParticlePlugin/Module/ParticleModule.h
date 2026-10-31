#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezProcessingStream;

/// Base class for all particle system modules
///
/// Modules process particle data through streams.
/// Derived types include emitters, initializers, behaviors, and finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleModule : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleModule, ezProcessingStreamProcessor);

  friend class ezParticleSystemInstance;

public:
  virtual void CreateRequiredStreams() = 0;
  virtual void QueryOptionalStreams() {}

  void Reset(ezParticleSystemInstance* pOwner)
  {
    m_pOwnerSystem = pOwner;
    m_StreamBinding.Clear();
    m_bUpdateFromWorld = false;

    OnReset();
  }

  /// Called after everything is set up.
  virtual void OnFinalize() {}

  ezParticleSystemInstance* GetOwnerSystem() { return m_pOwnerSystem; }

  const ezParticleSystemInstance* GetOwnerSystem() const { return m_pOwnerSystem; }

  ezParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerSystem->GetOwnerEffect(); }

  /// Override this to cache world module pointers for later access.
  ///
  /// Cached modules can be retrieved via ezParticleWorldModule::GetCachedWorldModule().
  virtual void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) {}

  /// Gives the module access to the world, for modules that need data from other objects.
  ///
  /// Only called once the module called RequestUpdateFromWorld().
  /// Called on the main thread once per frame in the PostTransform phase, after the particle update tasks have finished and before the next update.
  /// Data written here can therefore be read during the next update without further synchronization.
  /// hOwnerObject is the object that the effect is attached to (see ezParticleEffectInstance::SetOwnerObject()).
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) {}

  bool GetUpdateFromWorld() const { return m_bUpdateFromWorld; }

protected:
  /// Enables calls to UpdateFromWorld(). Reset() disables it again, so this is typically called in OnFinalize().
  void RequestUpdateFromWorld() { m_bUpdateFromWorld = true; }

  /// Called by Reset() to perform custom cleanup
  virtual void OnReset() {}

  void CreateStream(const char* szName, ezProcessingStream::DataType Type, ezProcessingStream** ppStream, bool bWillInitializeStream)
  {
    m_pOwnerSystem->CreateStream(szName, Type, ppStream, m_StreamBinding, bWillInitializeStream);
  }

  virtual ezResult UpdateStreamBindings() final override
  {
    m_StreamBinding.UpdateBindings(m_pStreamGroup);
    return EZ_SUCCESS;
  }

  ezRandom& GetRNG() const { return GetOwnerEffect()->GetRNG(); }

private:
  ezParticleSystemInstance* m_pOwnerSystem;
  ezParticleStreamBinding m_StreamBinding;
  bool m_bUpdateFromWorld = false;
};
