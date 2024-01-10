
template<typename T>
ezCustomDataResource<T>::ezCustomDataResource() = default;

template<typename T>
ezCustomDataResource<T>::~ezCustomDataResource() = default;

template<typename T>
void ezCustomDataResource<T>::CreateAndLoadData(ezAbstractObjectGraph& graph, ezRttiConverterContext& context, const ezAbstractObjectNode* pRootNode)
{
  T* pData = reinterpret_cast<T*>(m_Data);

  if (GetLoadingState() == ezResourceState::Loaded)
    pData->~T();

  new (m_Data) T();

  pData->Load(graph, context, pRootNode);
}

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UnloadData(Unload WhatToUnload)
{
  if (GetData() != nullptr)
    GetData()->~T();
  return ezCustomDataResourceBase::UnloadData(WhatToUnload);
}

template<typename T>
ezResourceLoadDesc ezCustomDataResource<T>::UpdateContent(ezStreamReader* Stream)
{
  return UpdateContent_Internal(Stream, *ezGetStaticRTTI<T>());
}

template<typename T>
void ezCustomDataResource<T>::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezCustomDataResource<T>);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}
