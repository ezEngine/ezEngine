#pragma once

#include <RendererCore/Declarations.h>
#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

struct EZ_PARTICLEPLUGIN_DLL ezParticleEffectResourceDescriptor
{
  virtual void Save(ezStreamWriter& stream) const;
  virtual void Load(ezStreamReader& stream);

  ezParticleEffectDescriptor m_Effect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectResource : public ezResource<ezParticleEffectResource, ezParticleEffectResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectResource, ezResourceBase);

public:
  ezParticleEffectResource();
  ~ezParticleEffectResource();

  const ezParticleEffectResourceDescriptor& GetDescriptor() { return m_Desc; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(ezParticleEffectResourceDescriptor&& descriptor) override;

private:
  ezParticleEffectResourceDescriptor m_Desc;


};
