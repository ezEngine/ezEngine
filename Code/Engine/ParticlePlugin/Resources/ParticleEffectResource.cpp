#include <PCH.h>
#include <Core/Assets/AssetFileHeader.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectResource, 1, ezRTTIDefaultAllocator<ezParticleEffectResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEffectResource::ezParticleEffectResource()
  : ezResource<ezParticleEffectResource, ezParticleEffectResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezParticleEffectResource::~ezParticleEffectResource()
{
}

ezResourceLoadDesc ezParticleEffectResource::UnloadData(Unload WhatToUnload)
{
  /// \todo Clear something
  //m_Desc.m_System1

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezParticleEffectResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Desc.Load(*Stream);

  return res;
}

void ezParticleEffectResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  /// \todo Better statistics
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezParticleEffectResource) + sizeof(ezParticleEffectResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezParticleEffectResource::CreateResource(const ezParticleEffectResourceDescriptor& descriptor)
{
  m_Desc = descriptor;

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

void ezParticleEffectResourceDescriptor::Save(ezStreamWriter& stream) const
{
  m_Effect.Save(stream);
}

void ezParticleEffectResourceDescriptor::Load(ezStreamReader& stream)
{
  m_Effect.Load(stream);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Resources_ParticleEffectResource);

