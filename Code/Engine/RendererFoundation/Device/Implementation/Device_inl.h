
/// \brief Used to guard ezGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define EZ_GALDEVICE_LOCK_AND_CHECK()                                                                                                                \
  EZ_LOCK(m_Mutex);                                                                                                                                  \
  VerifyMultithreadedAccess()

EZ_ALWAYS_INLINE const ezGALDeviceCreationDescription* ezGALDevice::GetDescription() const
{
  return &m_Description;
}

EZ_ALWAYS_INLINE ezResult ezGALDevice::GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& result)
{
  return GetTimestampResultPlatform(hTimestamp, result);
}

EZ_ALWAYS_INLINE ezGALTimestampHandle ezGALDevice::GetTimestamp()
{
  return GetTimestampPlatform();
}

EZ_ALWAYS_INLINE ezGALSwapChainHandle ezGALDevice::GetPrimarySwapChain() const
{
  return m_hPrimarySwapChain;
}

template <typename IdTableType, typename ReturnType>
EZ_ALWAYS_INLINE ReturnType* ezGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  EZ_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

inline const ezGALSwapChain* ezGALDevice::GetSwapChain(ezGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, ezGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const ezGALShader* ezGALDevice::GetShader(ezGALShaderHandle hShader) const
{
  return Get<ShaderTable, ezGALShader>(hShader, m_Shaders);
}

inline const ezGALTexture* ezGALDevice::GetTexture(ezGALTextureHandle hTexture) const
{
  return Get<TextureTable, ezGALTexture>(hTexture, m_Textures);
}

inline const ezGALBuffer* ezGALDevice::GetBuffer(ezGALBufferHandle hBuffer) const
{
  return Get<BufferTable, ezGALBuffer>(hBuffer, m_Buffers);
}

inline const ezGALDepthStencilState* ezGALDevice::GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, ezGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const ezGALBlendState* ezGALDevice::GetBlendState(ezGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, ezGALBlendState>(hBlendState, m_BlendStates);
}

inline const ezGALRasterizerState* ezGALDevice::GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, ezGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const ezGALVertexDeclaration* ezGALDevice::GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, ezGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

inline const ezGALSamplerState* ezGALDevice::GetSamplerState(ezGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, ezGALSamplerState>(hSamplerState, m_SamplerStates);
}

inline const ezGALResourceView* ezGALDevice::GetResourceView(ezGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, ezGALResourceView>(hResourceView, m_ResourceViews);
}

inline const ezGALRenderTargetView* ezGALDevice::GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, ezGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

inline const ezGALUnorderedAccessView* ezGALDevice::GetUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<UnorderedAccessViewTable, ezGALUnorderedAccessView>(hUnorderedAccessView, m_UnorderedAccessViews);
}

inline const ezGALFence* ezGALDevice::GetFence(ezGALFenceHandle hFence) const
{
  return Get<FenceTable, ezGALFence>(hFence, m_Fences);
}

inline const ezGALQuery* ezGALDevice::GetQuery(ezGALQueryHandle hQuery) const
{
  return Get<QueryTable, ezGALQuery>(hQuery, m_Queries);
}

// static
EZ_ALWAYS_INLINE void ezGALDevice::SetDefaultDevice(ezGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
EZ_ALWAYS_INLINE ezGALDevice* ezGALDevice::GetDefaultDevice()
{
  EZ_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");
  return s_pDefaultDevice;
}

// static
EZ_ALWAYS_INLINE bool ezGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
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
      m_DeadObjects.RemoveAtAndCopy(i);
      return;
    }
  }
}

EZ_ALWAYS_INLINE void ezGALDevice::VerifyMultithreadedAccess() const
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_DEV(m_Capabilities.m_bMultithreadedResourceCreation || ezThreadUtils::IsMainThread(),
    "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
