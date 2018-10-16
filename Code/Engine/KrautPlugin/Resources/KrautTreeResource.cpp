#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeResource, 1, ezRTTIDefaultAllocator<ezKrautTreeResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeResource::ezKrautTreeResource()
    : ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire KrautTree
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::Unloaded;
  }

  return res;
}

ezResourceLoadDesc ezKrautTreeResource::UpdateContent(ezStreamReader* Stream)
{
  ezKrautTreeResourceDescriptor desc;
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(desc);
}

void ezKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezKrautTreeResource::CreateResource(const ezKrautTreeResourceDescriptor& desc)
{
  m_Descriptor = desc;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeResourceDescriptor::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 1;

  stream << uiVersion;
  stream << m_Positions;
  stream << m_TriangleIndices;
}

ezResult ezKrautTreeResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;
  stream >> m_Positions;
  stream >> m_TriangleIndices;
  return EZ_SUCCESS;
}
