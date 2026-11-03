#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectResource, 1, ezRTTIDefaultAllocator<ezParticleEffectResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezParticleEffectResource);
// clang-format on

ezParticleEffectResource::ezParticleEffectResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
  m_pDesc = EZ_DEFAULT_NEW(ezParticleEffectResourceDescriptor);
}

ezParticleEffectResource::~ezParticleEffectResource() = default;

const ezSharedPtr<const ezParticleEffectResourceDescriptor>& ezParticleEffectResource::GetDescriptor() const
{
  EZ_LOCK(m_DescMutex);
  return m_pDesc;
}

void ezParticleEffectResource::SetDescriptor(const ezSharedPtr<const ezParticleEffectResourceDescriptor>& pDesc)
{
  EZ_LOCK(m_DescMutex);
  m_pDesc = pDesc;
}

ezResourceLoadDesc ezParticleEffectResource::UnloadData(Unload WhatToUnload)
{
  /// \todo Clear something
  // m_Desc.m_System1

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
  AssetHash.Read(*Stream).IgnoreResult();

  ezSharedPtr<ezParticleEffectResourceDescriptor> pDesc = EZ_DEFAULT_NEW(ezParticleEffectResourceDescriptor);
  pDesc->Load(*Stream);
  SetDescriptor(pDesc);

  return res;
}

void ezParticleEffectResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  /// \todo Better statistics
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezParticleEffectResource) + sizeof(ezParticleEffectResourceDescriptor);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezParticleEffectResource, ezParticleEffectResourceDescriptor)
{
  ezSharedPtr<ezParticleEffectResourceDescriptor> pDesc = EZ_DEFAULT_NEW(ezParticleEffectResourceDescriptor);
  *pDesc = descriptor;
  SetDescriptor(pDesc);

  ezResourceLoadDesc res;
  res.m_State = ezResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

void ezParticleEffectResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  m_Effect.Save(inout_stream);
}

void ezParticleEffectResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  m_Effect.Load(inout_stream);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Resources_ParticleEffectResource);
