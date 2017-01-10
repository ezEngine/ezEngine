
/// \brief Used to guard ezGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define EZ_GALDEVICE_LOCK_AND_CHECK() \
  EZ_LOCK(m_Mutex); \
  VerifyMultithreadedAccess()

EZ_FORCE_INLINE const ezGALDeviceCreationDescription* ezGALDevice::GetDescription() const
{
  return &m_Description;
}

EZ_FORCE_INLINE ezGALSwapChainHandle ezGALDevice::GetPrimarySwapChain() const
{
  return m_hPrimarySwapChain;
}

EZ_FORCE_INLINE ezGALContext* ezGALDevice::GetPrimaryContext() const
{
  return m_pPrimaryContext;
}

template<typename T>
EZ_FORCE_INLINE T* ezGALDevice::GetPrimaryContext() const
{
  return static_cast<T*>(m_pPrimaryContext);
}


template<typename IdTableType, typename ReturnType>
EZ_FORCE_INLINE ReturnType* ezGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

EZ_FORCE_INLINE const ezGALSwapChain* ezGALDevice::GetSwapChain(ezGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, ezGALSwapChain>(hSwapChain, m_SwapChains);
}

EZ_FORCE_INLINE const ezGALShader* ezGALDevice::GetShader(ezGALShaderHandle hShader) const
{
  return Get<ShaderTable, ezGALShader>(hShader, m_Shaders);
}

EZ_FORCE_INLINE const ezGALTexture* ezGALDevice::GetTexture(ezGALTextureHandle hTexture) const
{
  return Get<TextureTable, ezGALTexture>(hTexture, m_Textures);
}

EZ_FORCE_INLINE const ezGALBuffer* ezGALDevice::GetBuffer(ezGALBufferHandle hBuffer) const
{
  return Get<BufferTable, ezGALBuffer>(hBuffer, m_Buffers);
}

EZ_FORCE_INLINE const ezGALDepthStencilState* ezGALDevice::GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, ezGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

EZ_FORCE_INLINE const ezGALBlendState* ezGALDevice::GetBlendState(ezGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, ezGALBlendState>(hBlendState, m_BlendStates);
}

EZ_FORCE_INLINE const ezGALRasterizerState* ezGALDevice::GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, ezGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

EZ_FORCE_INLINE const ezGALVertexDeclaration* ezGALDevice::GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, ezGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

EZ_FORCE_INLINE const ezGALSamplerState* ezGALDevice::GetSamplerState(ezGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, ezGALSamplerState>(hSamplerState, m_SamplerStates);
}

EZ_FORCE_INLINE const ezGALResourceView* ezGALDevice::GetResourceView(ezGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, ezGALResourceView>(hResourceView, m_ResourceViews);
}

EZ_FORCE_INLINE const ezGALRenderTargetView* ezGALDevice::GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, ezGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

EZ_FORCE_INLINE const ezGALUnorderedAccessView* ezGALDevice::GetUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<UnorderedAccessViewTable, ezGALUnorderedAccessView>(hUnorderedAccessView, m_UnorderedAccessViews);
}

EZ_FORCE_INLINE const ezGALFence* ezGALDevice::GetFence(ezGALFenceHandle hFence) const
{
  return Get<FenceTable, ezGALFence>(hFence, m_Fences);
}

EZ_FORCE_INLINE const ezGALQuery* ezGALDevice::GetQuery(ezGALQueryHandle hQuery) const
{
  return Get<QueryTable, ezGALQuery>(hQuery, m_Queries);
}

// static
EZ_FORCE_INLINE void ezGALDevice::SetDefaultDevice(ezGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
EZ_FORCE_INLINE ezGALDevice* ezGALDevice::GetDefaultDevice()
{
  EZ_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");
  return s_pDefaultDevice;
}

template <typename HandleType>
EZ_FORCE_INLINE void ezGALDevice::AddDeadObject(ezUInt32 uiType, HandleType handle)
{
  auto& deadObject = m_DeadObjects.ExpandAndGetRef();
  deadObject.m_uiType = uiType;
  deadObject.m_uiHandle = handle.GetInternalID().m_Data;
}

template <typename HandleType>
void ezGALDevice::ReviveDeadObject(ezUInt32 uiType, HandleType handle)
{
  ezUInt32 uiHandle = handle.GetInternalID().m_Data;

  for (ezUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    if (deadObject.m_uiType == uiType && deadObject.m_uiHandle == uiHandle)
    {
      m_DeadObjects.RemoveAt(i);
      return;
    }
  }
}

EZ_FORCE_INLINE void ezGALDevice::VerifyMultithreadedAccess() const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_DEV(m_Capabilities.m_bMultithreadedResourceCreation || ezThreadUtils::IsMainThread(),
    "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
