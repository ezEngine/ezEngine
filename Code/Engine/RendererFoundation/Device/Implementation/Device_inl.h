
#include <Foundation/Logging/Log.h>

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
  ReturnType* pObject = nullptr;

  if(IdTable.TryGetValue(hHandle, pObject))
  {
    return pObject;
  }
  else
  {
    ezLog::Warning("ezGALDevice::Get() for invalid handle!");
    /// \todo typeid of incomplete types not allowed. Fix includes or move this away from the inline file.
    //ezLog::Warning("ezGALDevice::Get() for invalid handle for object type %s", typeid(ReturnType).name());
    return nullptr;
  }
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

EZ_FORCE_INLINE const ezGALQuery* ezGALDevice::GetQuery(ezGALQueryHandle hQuery) const
{
  return Get<QueryTable, ezGALQuery>(hQuery, m_Queries);
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
