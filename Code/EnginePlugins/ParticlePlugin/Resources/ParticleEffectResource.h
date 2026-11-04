#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

/// Descriptor for particle effect resources
///
/// Contains the particle effect configuration data.
/// Reference counted, because running effects use the modules in here directly, see ezParticleEffectResource::GetDescriptor().
struct EZ_PARTICLEPLUGIN_DLL ezParticleEffectResourceDescriptor : public ezRefCounted
{
  virtual void Save(ezStreamWriter& inout_stream) const;
  virtual void Load(ezStreamReader& inout_stream);

  ezParticleEffectDescriptor m_Effect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectResource final : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezParticleEffectResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezParticleEffectResource, ezParticleEffectResourceDescriptor);

public:
  ezParticleEffectResource();
  ~ezParticleEffectResource();

  /// Returns the current content of the resource. Immutable, a resource reload creates a new descriptor.
  ///
  /// The resource can be reloaded on another thread at any time, so the returned pointer has to be kept for as long as the data is used.
  /// In particular, don't bind references to data inside of it, without storing the pointer as well.
  const ezSharedPtr<const ezParticleEffectResourceDescriptor>& GetDescriptor() const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  void SetDescriptor(const ezSharedPtr<const ezParticleEffectResourceDescriptor>& pDesc);

  mutable ezMutex m_DescMutex;
  ezSharedPtr<const ezParticleEffectResourceDescriptor> m_pDesc;
};
