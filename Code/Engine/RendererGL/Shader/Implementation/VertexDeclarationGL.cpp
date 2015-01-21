#include <RendererGL/PCH.h>
#include <RendererGL/Shader/ShaderGL.h>
#include <RendererGL/Shader/VertexDeclarationGL.h>
#include <RendererGL/Device/DeviceGL.h>
#include <Foundation/Logging/Log.h>

#include <RendererGL/glew/glew.h>

ezGALVertexDeclarationGL::ezGALVertexDeclarationGL(const ezGALVertexDeclarationCreationDescription& Description)
  : ezGALVertexDeclaration(Description)
{
  m_AttributesGL.SetCount(Description.m_VertexAttributes.GetCount());
}

ezGALVertexDeclarationGL::~ezGALVertexDeclarationGL()
{
}

ezResult ezGALVertexDeclarationGL::InitPlatform(ezGALDevice* pDevice)
{

  // Need to determine max vertex attribs somewhere
 // EZ_ASSERT_DEV(m_Description.m_VertexAttributes.GetCount() <= GL_MAX_VERTEX_ATTRIBUTES, "Too many vertex attributes! Only %i supported, %i were requested.", GL_MAX_VERTEX_ATTRIBUTES, m_Description.m_VertexAttributes.GetCount());

  ezGALDeviceGL* pDeviceGL = static_cast<ezGALDeviceGL*>(pDevice);

  // Compute strides.
  ezUInt32 uiStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
    uiStrides[i] = 0;
  for (ezUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); ++i)
  {
    const ezGALVertexAttribute& vertexAttrib = m_Description.m_VertexAttributes[i];
    uiStrides[vertexAttrib.m_uiVertexBufferSlot] += ezGALResourceFormat::GetSize(vertexAttrib.m_eFormat);
  }

  for (ezUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); ++i)
  {
    const ezGALVertexAttribute& vertexAttrib = m_Description.m_VertexAttributes[i];
  
    ezGALFormatLookupEntryGL formatInfo = pDeviceGL->GetFormatLookupTable().GetFormatInfo(vertexAttrib.m_eFormat);
    if (formatInfo.m_eVertexAttributeType == EZ_RENDERERGL_INVALID_ID)
    {
      ezLog::Error("Format %i is not supported as vertex attribute type!", vertexAttrib.m_eFormat);
      return EZ_FAILURE;
    }

    /// \todo How to handle exotic formats like GL_INT_2_10_10_10_REV? They need channel count 1!

    m_AttributesGL[i].uiBufferIndex = vertexAttrib.m_uiVertexBufferSlot;
    m_AttributesGL[i].uiStride = uiStrides[vertexAttrib.m_uiVertexBufferSlot];
    m_AttributesGL[i].uiOffset = vertexAttrib.m_uiOffset;
    m_AttributesGL[i].uiChannelCount = ezGALResourceFormat::GetChannelCount(vertexAttrib.m_eFormat);
    m_AttributesGL[i].bNormalized = true; /// \todo
    m_AttributesGL[i].uiDivisor = vertexAttrib.m_bInstanceData ? 0 : 1;
    m_AttributesGL[i].uiFormat = formatInfo.m_eVertexAttributeType;
  }

  return EZ_SUCCESS;
}

ezResult ezGALVertexDeclarationGL::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}