
template <typename T>
ezCustomDataResource<T>::ezCustomDataResource() = default;

template <typename T>
ezCustomDataResource<T>::~ezCustomDataResource() = default;

template <typename T>
void ezCustomDataResource<T>::CreateAndLoadData(ezAbstractObjectGraph& ref_graph, ezRttiConverterContext& ref_context, const ezAbstractObjectNode* pRootNode)
{
  T* pData = reinterpret_cast<T*>(m_Data);

  if (GetLoadingState() == ezResourceState::Loaded)
  {
    ezMemoryUtils::Destruct(pData);
  }

  ezMemoryUtils::Construct<SkipTrivialTypes>(pData);

  if (pRootNode)
  {
    // pRootNode is empty when the resource file is empty
    // no need to attempt to load it then
    pData->Load(ref_graph, ref_context, pRootNode);
  }
}

template <typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  if (GetData() != nullptr)
  {
    ezMemoryUtils::Destruct(GetData());
  }

  return ezCustomDataResourceBase::UnloadData(WhatToUnload);
}

template <typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UpdateContent(ezStreamReader* Stream)
{
  return UpdateContent_Internal(Stream, *ezGetStaticRTTI<T>());
}

template <typename T>
void ezCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}
