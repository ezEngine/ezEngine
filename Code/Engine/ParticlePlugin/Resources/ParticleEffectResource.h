#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

struct EZ_PARTICLEPLUGIN_DLL ezParticleEffectResourceDescriptor
{
  virtual void Save(ezStreamWriter& stream) const;
  virtual void Load(ezStreamReader& stream);

  ezParticleEffectDescriptor m_Effect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezParticleEffectResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezParticleEffectResource, ezParticleEffectResourceDescriptor);

public:
  ezParticleEffectResource();
  ~ezParticleEffectResource();

  const ezParticleEffectResourceDescriptor& GetDescriptor() { return m_Desc; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezParticleEffectResourceDescriptor m_Desc;
};
