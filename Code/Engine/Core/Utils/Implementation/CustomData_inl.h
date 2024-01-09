
template<typename T>
ezCustomDataResource<T>::ezCustomDataResource() : ezResource(DoUpdate::OnAnyThread, 1)
{
}

template<typename T>
ezCustomDataResource<T>::~ezCustomDataResource() = default;

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  return res;
}

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UpdateContent(ezStreamReader* Stream)
{
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
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    T dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

template<typename T>
void ezCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

template<typename T>
EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezCustomDataResource<T>, T)
{
  m_Data = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}
