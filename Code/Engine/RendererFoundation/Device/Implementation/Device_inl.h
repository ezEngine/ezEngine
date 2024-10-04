
/// \brief Used to guard ezGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define EZ_GALDEVICE_LOCK_AND_CHECK() \
  EZ_LOCK(m_Mutex);                   \
  VerifyMultithreadedAccess()

EZ_ALWAYS_INLINE const ezGALDeviceCreationDescription* ezGALDevice::GetDescription() const
{
  return &m_Description;
}

EZ_ALWAYS_INLINE ezUInt64 ezGALDevice::GetCurrentFrame() const
{
  return GetCurrentFramePlatform();
}

EZ_ALWAYS_INLINE ezUInt64 ezGALDevice::GetSafeFrame() const
{
  return GetSafeFramePlatform();
}

EZ_ALWAYS_INLINE ezEnum<ezGALAsyncResult> ezGALDevice::GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& out_result)
{
  if (hTimestamp.IsInvalidated())
    return ezGALAsyncResult::Expired;

  return GetTimestampResultPlatform(hTimestamp, out_result);
}

EZ_ALWAYS_INLINE ezEnum<ezGALAsyncResult> ezGALDevice::GetOcclusionQueryResult(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult)
{
  if (hOcclusion.IsInvalidated())
    return ezGALAsyncResult::Expired;

  return GetOcclusionResultPlatform(hOcclusion, out_uiResult);
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

inline const ezGALTextureResourceView* ezGALDevice::GetResourceView(ezGALTextureResourceViewHandle hResourceView) const
{
  return Get<TextureResourceViewTable, ezGALTextureResourceView>(hResourceView, m_TextureResourceViews);
}

inline const ezGALBufferResourceView* ezGALDevice::GetResourceView(ezGALBufferResourceViewHandle hResourceView) const
{
  return Get<BufferResourceViewTable, ezGALBufferResourceView>(hResourceView, m_BufferResourceViews);
}

inline const ezGALRenderTargetView* ezGALDevice::GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, ezGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

inline const ezGALTextureUnorderedAccessView* ezGALDevice::GetUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<TextureUnorderedAccessViewTable, ezGALTextureUnorderedAccessView>(hUnorderedAccessView, m_TextureUnorderedAccessViews);
}

inline const ezGALBufferUnorderedAccessView* ezGALDevice::GetUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<BufferUnorderedAccessViewTable, ezGALBufferUnorderedAccessView>(hUnorderedAccessView, m_BufferUnorderedAccessViews);
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

inline ezAllocator* ezGALDevice::GetAllocator()
{
  return &m_Allocator;
}
