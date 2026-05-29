#pragma once

ezRenderPipelinePinConnection::ezRenderPipelinePinConnection(Connectivity connectivity)
  : m_Connectivity(connectivity)
  , m_TextureHandle()
{
}

ezRenderPipelinePinConnection::ezRenderPipelinePinConnection(Connectivity connectivity, ezRenderGraphTextureHandle hTextureHandle)
  : m_Connectivity(connectivity)
  , m_TextureHandle(hTextureHandle)
{
}

ezRenderPipelinePinConnection::ezRenderPipelinePinConnection(Connectivity connectivity, ezRenderGraphBufferHandle hBufferHandle)
  : m_Connectivity(connectivity)
  , m_BufferHandle(hBufferHandle)
{
}

ezRenderPipelinePinConnection::ezRenderPipelinePinConnection(const ezRenderPipelinePinConnection& other)
  : m_Connectivity(other.m_Connectivity)
{
  switch (other.m_Connectivity)
  {
    case Connectivity::Texture:
      m_TextureHandle = other.m_TextureHandle;
      break;
    case Connectivity::Buffer:
      m_BufferHandle = other.m_BufferHandle;
      break;
    default:
      break;
  }
}

ezRenderPipelinePinConnection& ezRenderPipelinePinConnection::operator=(const ezRenderPipelinePinConnection& other)
{
  if (this != &other)
  {
    const_cast<Connectivity&>(m_Connectivity) = other.m_Connectivity;
    switch (m_Connectivity)
    {
      case Connectivity::Texture:
        m_TextureHandle = other.m_TextureHandle;
        break;
      case Connectivity::Buffer:
        m_BufferHandle = other.m_BufferHandle;
        break;
      default:
        break;
    }
  }
  return *this;
}