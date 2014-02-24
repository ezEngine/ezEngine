
#include <Foundation/Logging/Log.h>

const ezGALDeviceCreationDescription* ezGALDevice::GetDescription() const
{
  return &m_Description;
}

ezGALSwapChainHandle ezGALDevice::GetPrimarySwapChain() const
{
  return m_hPrimarySwapChain;
}

ezGALContext* ezGALDevice::GetPrimaryContext() const
{
  return m_pPrimaryContext;
}

template<typename T> T* ezGALDevice::GetPrimaryContext() const
{
  return static_cast<T*>(m_pPrimaryContext);
}


template<typename IdTableType, typename ReturnType> ReturnType* ezGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  ReturnType* pObject = NULL;

  if(IdTable.TryGetValue(hHandle, pObject))
  {
    return pObject;
  }
  else
  {
    ezLog::Warning("ezGALDevice::Get() for invalid handle for object type %s", typeid(ReturnType).name());
    return NULL;
  }
}

const ezGALSwapChain* ezGALDevice::GetSwapChain(ezGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, ezGALSwapChain>(hSwapChain, m_SwapChains);
}

const ezGALShader* ezGALDevice::GetShader(ezGALShaderHandle hShader) const
{
  return Get<ShaderTable, ezGALShader>(hShader, m_Shaders);
}

const ezGALTexture* ezGALDevice::GetTexture(ezGALTextureHandle hTexture) const
{
  return Get<TextureTable, ezGALTexture>(hTexture, m_Textures);
}

const ezGALBuffer* ezGALDevice::GetBuffer(ezGALBufferHandle hBuffer) const
{
  return Get<BufferTable, ezGALBuffer>(hBuffer, m_Buffers);
}

const ezGALDepthStencilState* ezGALDevice::GetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, ezGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

const ezGALBlendState* ezGALDevice::GetBlendState(ezGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, ezGALBlendState>(hBlendState, m_BlendStates);
}

const ezGALRasterizerState* ezGALDevice::GetRasterizerState(ezGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, ezGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

const ezGALRenderTargetConfig* ezGALDevice::GetRenderTargetConfig(ezGALRenderTargetConfigHandle hRenderTargetConfig) const
{
  return Get<RenderTargetConfigTable, ezGALRenderTargetConfig>(hRenderTargetConfig, m_RenderTargetConfigs);
}

const ezGALVertexDeclaration* ezGALDevice::GetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, ezGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

const ezGALQuery* ezGALDevice::GetQuery(ezGALQueryHandle hQuery) const
{
  return Get<QueryTable, ezGALQuery>(hQuery, m_Queries);
}

const ezGALSamplerState* ezGALDevice::GetSamplerState(ezGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, ezGALSamplerState>(hSamplerState, m_SamplerStates);
}

const ezGALResourceView* ezGALDevice::GetResourceView(ezGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, ezGALResourceView>(hResourceView, m_ResourceViews);
}

const ezGALRenderTargetView* ezGALDevice::GetRenderTargetView(ezGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, ezGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}
