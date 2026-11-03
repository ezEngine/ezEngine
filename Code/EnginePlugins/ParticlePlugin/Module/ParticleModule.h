#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

/// Base class for particle modules, which are shared between all instances of an effect.
///
/// The modules are owned by the effect resource and are used from multiple particle update tasks at the same time, so all functions are const.
/// They may also be used by multiple worlds, so they must not store anything that belongs to a world.
/// Everything that is per system instance goes into a small struct, see GetInstanceDataDesc().
///
/// Instance data is zero-filled before it is constructed. It stays alive across a reconfigure of the effect, as long as the system keeps the same
/// module types, even if the modules were replaced by a resource reload.
/// So its layout may only depend on the type of the module, and modules must reset all state that shouldn't survive that in OnFinalize().
class EZ_PARTICLEPLUGIN_DLL ezParticleModule : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleModule, ezReflectedClass);

public:
  /// Describes the data that each system instance needs for this module. Returns false if there is none.
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }

  ezUInt32 GetInstanceDataOffset() const { return m_uiInstanceDataOffset; }

  /// Modules with a lower priority are updated first. The order of modules with the same priority is not defined.
  float GetPriority() const { return m_fPriority; }

  /// Override this to cache world module pointers for later access.
  ///
  /// Called once on a temporary instance of each type, for every world. Cached modules can be retrieved via
  /// ezParticleWorldModule::GetCachedWorldModule(). The module can't store them, as it may be used by multiple worlds.
  virtual void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const {}

  /// Called every time the system is configured.
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const {}

  /// Called once all modules have created their required streams.
  virtual void QueryOptionalStreams(const ezParticleModuleContext& ctx) const {}

  /// Called at the end of every configuration of the system, so after a reconfigure as well.
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const {}

  /// Called before the instance data is destructed, for cleanup that needs access to the system.
  ///
  /// The destructor of the instance data only gets the object itself. All streams still exist at this point.
  virtual void OnDeinitialize(const ezParticleModuleContext& ctx) const {}

  /// If this returns true, UpdateFromWorld() is called for this module.
  virtual bool GetUpdateFromWorld() const { return false; }

  /// Gives the module access to the world, for modules that need data from other objects, e.g. a spawn area or a mesh to spawn particles on.
  ///
  /// Called on the main thread once per frame in the PostTransform phase, after the particle update tasks have finished and before the next update.
  /// After the effect was created or reconfigured, it is additionally called once before its first update, so the data is available right away.
  /// Data written here can therefore be read during the next update without further synchronization.
  /// hOwnerObject is the object that the effect is attached to (see ezParticleEffectInstance::SetOwnerObject()).
  /// It is invalid for shared effects and for effects that were started without a particle component.
  virtual void UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const {}

  /// If this returns true, OnOwnerMessage() is called for this module.
  virtual bool GetReceiveOwnerMessages() const { return false; }

  /// Receives messages that the owner object of the effect forwards, see ezParticleEffectInstance::OnOwnerMessage().
  ///
  /// Messages may arrive during the 'Async' world update phase, while the particle update tasks are running, including the one of this module.
  /// Therefore the data must not be used right away, but stored as pending and made current in UpdateFromWorld().
  virtual void OnOwnerMessage(const ezParticleModuleContext& ctx, const ezMessage& msg) const {}

  /// If this returns true, OnParticleDeath() is called for this module.
  virtual bool GetReceiveParticleDeath() const { return false; }

  /// Called right before a particle gets removed. Its data can still be read from the streams.
  virtual void OnParticleDeath(const ezParticleModuleContext& ctx, ezUInt64 uiElementIndex) const {}

  /// Initializes the data of newly spawned particles.
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const {}

  /// Updates all active particles.
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const {}

protected:
  float m_fPriority = 0.0f;

private:
  friend class ezParticleSystemDescriptor;

  ezUInt32 m_uiInstanceDataOffset = ezInvalidIndex;
};

/// Connects all modules of one system instance to the stream group of that system.
///
/// ezProcessingStreamGroup requires an object per processor, which it owns. The modules themselves are shared between system instances,
/// so a single processor per system runs them all, in the order of their priorities.
/// For new particles it first runs the default initializers of those streams that no module initializes.
class EZ_PARTICLEPLUGIN_DLL ezParticleModuleProcessor final : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleModuleProcessor, ezProcessingStreamProcessor);

public:
  /// Sets up the update order. The instances must stay at the same memory location afterwards, their modules may be swapped.
  void SetModules(ezParticleSystemInstance* pSystem, ezArrayPtr<const ezParticleModuleInstance*> instances);

  /// Without a particle stream, the stream gets zero-initialized.
  void AddStreamInitializer(const ezParticleStream* pParticleStream, ezProcessingStream* pStream);
  void RemoveStreamInitializer(const ezProcessingStream* pStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;

private:
  struct StreamInitializer
  {
    EZ_DECLARE_POD_TYPE();

    const ezParticleStream* m_pParticleStream;
    ezProcessingStream* m_pStream;
  };

  ezParticleSystemInstance* m_pSystem = nullptr;
  ezSmallArray<const ezParticleModuleInstance*, 16> m_Instances;
  ezSmallArray<StreamInitializer, 8> m_StreamInitializers;
};
