
#include <RendererGL/PCH.h>
#include <RendererGL/Context/ContextGL.h>
#include <RendererGL/Device/DeviceGL.h>
#include <RendererGL/Shader/ShaderGL.h>
#include <RendererGL/Resources/BufferGL.h>
#include <RendererGL/Resources/TextureGL.h>
#include <RendererGL/Resources/RenderTargetConfigGL.h>
#include <RendererGL/Resources/RenderTargetViewGL.h>
#include <RendererGL/Shader/VertexDeclarationGL.h>
#include <RendererGL/State/StateGL.h>
#include <RendererGL/Resources/ResourceViewGL.h>

#include <RendererGL/glew/glew.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color.h>

/// Maps ezGALPrimitiveTopology to OpenGL types.
const ezUInt32 ezGALContextGL::s_GALTopologyToGL[ezGALPrimitiveTopology::ENUM_COUNT] =
{
  GL_TRIANGLES

  /// \todo Add other primitive types (when adding to ez enum)
};
/*
GL_POINTS,
GL_LINE_STRIP,
GL_LINE_LOOP,
GL_LINES,
GL_TRIANGLE_STRIP,
GL_TRIANGLE_FAN,
GL_TRIANGLES
GL_PATCHES    // Needs to call glPatchParameteri(GL_PATCH_VERTICES, i);
*/

const ezUInt32 ezGALContextGL::s_GALIndexTypeToGL[ezGALIndexType::ENUM_COUNT] =
{
  GL_UNSIGNED_SHORT,
  GL_UNSIGNED_INT
};

const ezUInt32 ezGALContextGL::s_GALBufferBindingToGL[ezGALContextGL::GLBufferBinding::ENUM_COUNT] =
{
  GL_SHADER_STORAGE_BUFFER,
  GL_ARRAY_BUFFER,
  GL_ELEMENT_ARRAY_BUFFER,
  GL_UNIFORM_BUFFER,

  GL_TRANSFORM_FEEDBACK_BUFFER,
  GL_DRAW_INDIRECT_BUFFER
};

const ezUInt32 ezGALContextGL::s_GALTextureTypeToGL[ezGALTextureType::ENUM_COUNT] =
{
  GL_TEXTURE_2D,                  // Texture2D
  GL_TEXTURE_CUBE_MAP_POSITIVE_X, // TextureCube - Note: Each cubemap size has to be defined separately. This is the side with the lowest index.
  GL_TEXTURE_3D                   // Texture3D
};


ezGALContextGL::ezGALContextGL(ezGALDeviceGL* pDevice)
  : ezGALContext(pDevice),
  m_PrimitiveTopology(ezGALPrimitiveTopology::Triangles),
  m_IndexType(ezGALIndexType::UShort),
  m_uiNumColorTarget(1),
  m_pBoundVertexAttributes(nullptr),
  m_uiNumGLVertexAttributesBound(0),

  m_DepthWriteMask(GL_TRUE),
  m_StencilReadMask(0xFFFFFFFF),
  m_StencilWriteMask(0xFFFFFFFF),

  m_CullFaceState(GL_BACK),
  m_PolygonMode(GL_FILL),

  m_iMaxNumTextureBindings(0),
  m_uiActiveTextureUnit(0)
{
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT; ++i)
  {
    m_BoundSamplerStates[i] = 0;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    m_BoundVertexBuffers[i] = 0;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; ++i)
  {
    m_VertexBufferStrides[i] = 0;
  }

  for (ezUInt32 i = 0; i < GLBufferBinding::ENUM_COUNT; ++i)
  {
    m_BufferBindings[i] = 0;
  }


  EZ_GL_CALL(glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_iMaxNumTextureBindings);

  for (ezUInt32 bindingType = 0; bindingType < ezGALTextureType::ENUM_COUNT; ++bindingType)
  {
    m_TextureBindings[bindingType] = EZ_DEFAULT_NEW_ARRAY(ezUInt32, m_iMaxNumTextureBindings);
    for (ezInt32 i = 0; i < m_iMaxNumTextureBindings; ++i)
    {
      m_TextureBindings[bindingType][i] = 0;
    }
  }
};

ezGALContextGL::~ezGALContextGL()
{
  for (ezInt32 bindingType = 0; bindingType < ezGALTextureType::ENUM_COUNT; ++bindingType)
  {
    EZ_DEFAULT_DELETE_ARRAY(m_TextureBindings[bindingType]);
  }
}

// State helper

ezResult ezGALContextGL::SetGLState(ezUInt32 uiStateIdentifier, bool on)
{
  bool* pbStateEntry = nullptr;
  if (m_EnableStates.TryGetValue(uiStateIdentifier, pbStateEntry))
  {
    if (*pbStateEntry == on)
    {
      return EZ_SUCCESS;
    }
    else if (on)
    {
      *pbStateEntry = true;
      return EZ_GL_CALL(glEnable, uiStateIdentifier);
    }
    else
    {
      *pbStateEntry = false;
      return EZ_GL_CALL(glDisable, uiStateIdentifier);
    }
  }
  else
  {
    // State yet unknown. Retrieve and call again.
    if (on != IsStateActive(uiStateIdentifier))
    {
      return SetGLState(uiStateIdentifier, on);
    }
  }

  return EZ_SUCCESS;
}

bool ezGALContextGL::IsStateActive(ezUInt32 uiStateIdentifier)
{
  bool bActive;
  if (!m_EnableStates.TryGetValue(uiStateIdentifier, bActive))
  {
    bActive = EZ_GL_RET_CALL(glIsEnabled, uiStateIdentifier) == GL_TRUE;
    m_EnableStates.Insert(uiStateIdentifier, bActive);
  }

  return bActive;
}

ezResult ezGALContextGL::BindBuffer(ezGALContextGL::GLBufferBinding::Enum binding, glBufferId buffer)
{
  if (m_BufferBindings[binding] != buffer)
  {
    m_BufferBindings[binding] = buffer;
    EZ_GL_CALL(glBindBuffer, s_GALBufferBindingToGL[binding], buffer);
  }
  return EZ_SUCCESS;
}

ezGALContextGL::ScopedBufferBinding::ScopedBufferBinding(ezGALContextGL* pContext, GLBufferBinding::Enum binding, glBufferId buffer) :
  m_pContext(pContext),
  m_binding(binding)
{
  EZ_ASSERT_DEV(pContext != nullptr, "Context is nullptr!");
  m_bufferBefore = pContext->m_BufferBindings[binding];
  m_pContext->BindBuffer(binding, buffer);
}

ezGALContextGL::ScopedBufferBinding::~ScopedBufferBinding()
{
  m_pContext->BindBuffer(m_binding, m_bufferBefore);
}


ezResult ezGALContextGL::BindTexture(ezGALTextureType::Enum textureType, glTextureId textureHandle, ezInt32 iBindingSlot)
{
  EZ_ASSERT_DEV(iBindingSlot < m_iMaxNumTextureBindings, "Can't bind texture on slot %i. Max number of texture slots available is %i!", iBindingSlot, m_iMaxNumTextureBindings);

  if (iBindingSlot >= 0)
  {
    if (m_uiActiveTextureUnit != iBindingSlot)
    {
      if (EZ_GL_CALL(glActiveTexture, iBindingSlot) != EZ_SUCCESS)
        return EZ_FAILURE;
      m_uiActiveTextureUnit = iBindingSlot;
    }
  }

  return EZ_GL_CALL(glBindTexture, s_GALTextureTypeToGL[textureType], textureHandle);
}

ezGALContextGL::ScopedTextureBinding::ScopedTextureBinding(ezGALContextGL* pContext, ezGALTextureType::Enum textureType, glTextureId texture) :
  m_pContext(pContext),
  m_textureType(textureType) 
{
  EZ_ASSERT_DEV(pContext != nullptr, "Context is nullptr!");

  m_uiUsedSlot = pContext->m_uiActiveTextureUnit;
  m_textureBefore = pContext->m_TextureBindings[m_textureType][m_uiUsedSlot];
  pContext->BindTexture(textureType, texture, -1);
}

ezGALContextGL::ScopedTextureBinding::~ScopedTextureBinding()
{
  m_pContext->BindTexture(m_textureType, m_textureBefore, m_uiUsedSlot);
}

// Draw functions

void ezGALContextGL::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  for (ezUInt32 i = 0; i < m_uiNumColorTarget; i++)
  {
    if (uiRenderTargetClearMask & (1u << i))
    {
      EZ_GL_CALL(glClearBufferfv, GL_COLOR, i, ClearColor.GetData());
    }
  }

  if (bClearDepth && bClearStencil)
  {
    EZ_GL_CALL(glClearBufferfi, GL_DEPTH_STENCIL, 0, fDepthClear, uiStencilClear);
  }
  else if (bClearDepth)
  {
    EZ_GL_CALL(glClearBufferfv, GL_DEPTH, 0, &fDepthClear);
  }
  else if (bClearStencil)
  {
    GLint iStencilClearGL = uiStencilClear;
    EZ_GL_CALL(glClearBufferiv, GL_STENCIL, 0, &iStencilClearGL);
  }
}

void ezGALContextGL::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  EZ_GL_CALL(glDrawArrays, s_GALTopologyToGL[m_PrimitiveTopology], uiStartVertex, uiVertexCount);
}

void ezGALContextGL::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();
  
  EZ_GL_CALL(glDrawElements, s_GALTopologyToGL[m_PrimitiveTopology], uiIndexCount, s_GALIndexTypeToGL[m_IndexType],
                  reinterpret_cast<GLvoid*>(ezGALIndexType::GetSize(m_IndexType) * uiStartIndex));
}

void ezGALContextGL::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  EZ_GL_CALL(glDrawElementsInstanced, s_GALTopologyToGL[m_PrimitiveTopology], uiIndexCountPerInstance, s_GALIndexTypeToGL[m_IndexType],
                  reinterpret_cast<GLvoid*>(ezGALIndexType::GetSize(m_IndexType) * uiStartIndex), uiInstanceCount);
}

void ezGALContextGL::DrawIndexedInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  EZ_GL_CALL(glDrawArraysInstanced, s_GALTopologyToGL[m_PrimitiveTopology], uiStartVertex, uiVertexCountPerInstance, uiInstanceCount);
}

void ezGALContextGL::DrawInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Some state changes are deferred so they can be updated faster
void ezGALContextGL::FlushDeferredStateChanges()
{
  if (m_DeferredStateChanged.IsSet(ezGALGL::DeferredStateChanged::VertexBuffer) || m_DeferredStateChanged.IsSet(ezGALGL::DeferredStateChanged::VertexDeclaration))
  {
    EZ_ASSERT_DEV(m_pBoundVertexAttributes != nullptr, "No vertex declaration is set!");

    /// \todo GL4 Together with glVertexAttribPointer
    //glBindVertexBuffers(0, EZ_GAL_MAX_VERTEX_BUFFER_COUNT, m_BoundVertexBuffers, nullptr, m_VertexBufferStrides);
    //EZ_RENDERERGL_CHECK_GL_ERROR("glBindVertexBuffers");

    ezInt32 iCurrentBufferIndex = -1;

    for (ezUInt32 i = 0; i < m_pBoundVertexAttributes->GetCount(); ++i)
    {
      const ezGALVertexDeclarationGL::VertexAttributeDesc& attribDesc = (*m_pBoundVertexAttributes)[i];

      if (attribDesc.uiBufferIndex != iCurrentBufferIndex)
      {
        BindBuffer(GLBufferBinding::VertexBuffer, m_BoundVertexBuffers[i]);
        iCurrentBufferIndex = static_cast<ezInt32>(attribDesc.uiBufferIndex);
      }

      EZ_GL_CALL(glVertexAttribPointer, i, attribDesc.uiChannelCount, attribDesc.uiFormat, attribDesc.bNormalized ? GL_TRUE : GL_FALSE,
                      attribDesc.uiStride, reinterpret_cast<void*>(attribDesc.uiOffset));
      EZ_GL_CALL(glEnableVertexAttribArray, i);
    }

    // Unbind all other vertex buffers.
    for (ezUInt32 i = m_pBoundVertexAttributes->GetCount(); i < m_uiNumGLVertexAttributesBound; ++i)
    {
      EZ_GL_CALL(glDisableVertexAttribArray, i);
    }

    m_uiNumGLVertexAttributesBound = m_pBoundVertexAttributes->GetCount();
  }

  if (m_DeferredStateChanged.IsSet(ezGALGL::DeferredStateChanged::SamplerState))
  {
    EZ_GL_CALL(glBindSamplers, 0, EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT, m_BoundSamplerStates);
  }

  m_DeferredStateChanged.Clear();
}

// Dispatch

void ezGALContextGL::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  EZ_GL_CALL(glDispatchCompute, uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALContextGL::DispatchIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

// State setting functions

void ezGALContextGL::SetShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderGL* pShaderGL = static_cast<ezGALShaderGL*>(pShader);
  EZ_GL_CALL(glUseProgram, pShaderGL->GetGLShaderProgram());
}

void ezGALContextGL::SetIndexBufferPlatform(ezGALBuffer* pIndexBuffer)
{
  // Retrieve index type.
  m_IndexType = ezGALIndexType::ENUM_COUNT;
  ezUInt32 uiIndexSize = pIndexBuffer->GetDescription().m_uiStructSize;
  for (unsigned int i = 0; i < ezGALIndexType::ENUM_COUNT; ++i)
  {
    if (uiIndexSize == ezGALIndexType::GetSize(static_cast<ezGALIndexType::Enum>(i)))
    {
      m_IndexType = static_cast<ezGALIndexType::Enum>(i);
      break;
    }
  }
  EZ_ASSERT_DEV(m_IndexType != ezGALIndexType::ENUM_COUNT, "Invalid index buffer element size: %i", uiIndexSize);

  ezGALBufferGL* pIndexBufferGL = static_cast<ezGALBufferGL*>(pIndexBuffer);
  BindBuffer(GLBufferBinding::IndexBuffer, pIndexBufferGL->GetGLBufferHandle());
}

void ezGALContextGL::SetVertexBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pVertexBuffer)
{
  ezGALBufferGL* pVertexBufferGL = static_cast<ezGALBufferGL*>(pVertexBuffer);

  m_BoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? pVertexBufferGL->GetGLBufferHandle() : 0;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;

  m_DeferredStateChanged.Add(ezGALGL::DeferredStateChanged::VertexBuffer);
}

void ezGALContextGL::SetVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  ezGALVertexDeclarationGL* pVertexDeclarationGL = static_cast<ezGALVertexDeclarationGL*>(pVertexDeclaration);
  m_pBoundVertexAttributes = &pVertexDeclarationGL->GetGLAttributeDesc();
}

void ezGALContextGL::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_PrimitiveTopology = Topology;
}

void ezGALContextGL::SetConstantBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer)
{
  glBufferId bufferHandle = 0;
  if (pBuffer != nullptr)
    bufferHandle = static_cast<ezGALBufferGL*>(pBuffer)->GetGLBufferHandle();
  
  EZ_GL_CALL(glBindBufferBase, GL_UNIFORM_BUFFER, uiSlot, bufferHandle);
  m_BufferBindings[GLBufferBinding::ConstantBuffer] = bufferHandle;
}

void ezGALContextGL::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerState* pSamplerState)
{
  m_BoundSamplerStates[uiSlot] = pSamplerState != nullptr ? static_cast<ezGALSamplerStateGL*>(pSamplerState)->GetGLSamplerState() : 0;
  m_DeferredStateChanged.Add(ezGALGL::DeferredStateChanged::SamplerState);
}

void ezGALContextGL::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceView* pResourceView)
{
  /// \todo GL4 TextureViews
  // For every other GL version there should be nothing left to do.
}

void ezGALContextGL::SetRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig)
{
 /* if (pRenderTargetConfig != nullptr)
  {
    ezGALRenderTargetConfigGL* pRenderTargetConfigGL = static_cast<ezGALRenderTargetConfigGL*>(pRenderTargetConfig);
    EZ_GL_CALL(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, pRenderTargetConfigGL->GetGLBufferHandle());

    m_uiNumColorTarget = pRenderTargetConfigGL->GetDescription().m_uiColorTargetCount;
  }
  else */
  {
    EZ_GL_CALL(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, 0);
    m_uiNumColorTarget = 1;
  }
}

void ezGALContextGL::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, ezGALResourceView* pResourceView)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::SetBlendStatePlatform(ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::SetDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  const ezGALDepthStencilStateCreationDescription& depthDesc = pDepthStencilState->GetDescription();

  SetGLState(GL_DEPTH_TEST, depthDesc.m_bDepthTest);
  SetGLState(GL_STENCIL_TEST, depthDesc.m_bStencilTest);

  ezUInt32 newDepthWriteMask = depthDesc.m_bDepthWrite ? GL_TRUE : GL_FALSE;
  if (m_DepthWriteMask != newDepthWriteMask)
  {
    m_DepthWriteMask = newDepthWriteMask;
    EZ_GL_CALL(glDepthMask, newDepthWriteMask);
  }

  if (m_StencilWriteMask != depthDesc.m_uiStencilWriteMask)
  {
    m_StencilWriteMask = depthDesc.m_uiStencilWriteMask;
    EZ_GL_CALL(glStencilMask, m_StencilWriteMask);
  }

  /// \todo Stencil func needs to be implemented

 /* ezGALStencilOpDescription m_FrontFaceStencilOp;
  ezGALStencilOpDescription m_BackFaceStencilOp;

  ezGALCompareFunc::Enum m_DepthTestFunc;

  bool m_bSeparateFrontAndBack; ///< If false, DX11 will use front face values for both front & back face values, GL will not call gl*Separate() funcs

  ezUInt8 m_uiStencilReadMask;*/
};

void ezGALContextGL::SetRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  const ezGALRasterizerStateCreationDescription& rasterDesc = pRasterizerState->GetDescription();

  // Cullmode
  switch (rasterDesc.m_CullMode)
  {
  case ezGALCullMode::None:
    SetGLState(GL_CULL_FACE, false);
    break;

  case ezGALCullMode::Front:
    SetGLState(GL_CULL_FACE, true);
    if (m_CullFaceState != GL_FRONT)
    {
      EZ_GL_CALL(glCullFace, GL_FRONT);
      m_CullFaceState = GL_FRONT;
    }
    break;

  case ezGALCullMode::Back:
    EZ_GL_CALL(glEnable, GL_CULL_FACE);
    if (m_CullFaceState != GL_BACK)
    {
      EZ_GL_CALL(glCullFace, GL_BACK);
      m_CullFaceState = GL_BACK;
    }
    break;

  default:
    EZ_REPORT_FAILURE("Cullmode %i is not implemented!", rasterDesc.m_CullMode);
  }

  // Wireframe
#ifndef EZ_RENDERERGL_GLES3
  if (rasterDesc.m_bWireFrame)
  {
    if (m_PolygonMode != GL_LINE)
    {
      EZ_GL_CALL(glPolygonMode, GL_FRONT_AND_BACK, GL_LINE);
      m_PolygonMode = GL_LINE;
    }
  }
  else
  {
    if (m_PolygonMode != GL_FILL)
    {
      EZ_GL_CALL(glPolygonMode, GL_FRONT_AND_BACK, GL_FILL);
      m_PolygonMode = GL_FILL;
    }
  }
#endif


  // MSAA
  if (rasterDesc.m_bMSAA)
  {
    SetGLState(GL_MULTISAMPLE, true);
  }
  else
  {
    SetGLState(GL_MULTISAMPLE, false);
  }

  /// \todo
    /*
  ezInt32 m_iDepthBias;
  float m_fDepthBiasClamp;
  float m_fSlopeScaledDepthBias;
  bool m_bFrontCounterClockwise;
  bool m_bDepthClip;
  bool m_bScissorTest;
  bool m_bLineAA;
  */
}

void ezGALContextGL::SetViewportPlatform(float fX, float fY, float fWidth, float fHeight, float fMinDepth, float fMaxDepth)
{
#ifdef EZ_RENDERERGL_GL4
  EZ_GL_CALL(glViewportIndexedf, 0, fX, fY, fWidth, fHeight);
#else
  EZ_GL_CALL(glViewport, static_cast<GLint>(fX), static_cast<GLint>(fY), static_cast<GLint>(fWidth), static_cast<GLint>(fHeight));
#endif

  EZ_GL_CALL(glDepthRangef, fMinDepth, fMaxDepth);
}

void ezGALContextGL::SetScissorRectPlatform(ezUInt32 uiX, ezUInt32 uiY, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  glScissor(uiX, uiY, uiWidth, uiHeight);
}

void ezGALContextGL::SetStreamOutBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Fence & Query functions

void ezGALContextGL::InsertFencePlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

bool ezGALContextGL::IsFenceReachedPlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void ezGALContextGL::BeginQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::EndQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Resource update functions

void ezGALContextGL::CopyBufferPlatform(ezGALBuffer* pDestination, ezGALBuffer* pSource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::CopyBufferRegionPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::UpdateBufferPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const void* pSourceData, ezUInt32 uiByteCount)
{
  ezGALBufferGL* pBufferGL = static_cast<ezGALBufferGL*>(pDestination);

  /// \todo All those glGetIntegerv should be avoided! Need to store gl state somewhere
  glBufferId bufferBoundBefore = EZ_RENDERERGL_INVALID_ID;
  EZ_GL_CALL(glGetIntegerv, GL_UNIFORM_BUFFER_BINDING, reinterpret_cast<GLint*>(&bufferBoundBefore));
  
  ScopedBufferBinding scopedBinding(this, GLBufferBinding::ConstantBuffer, pBufferGL->GetGLBufferHandle());
  EZ_GL_CALL(glBufferSubData, GL_UNIFORM_BUFFER, uiDestOffset, uiByteCount, pSourceData);
}

void ezGALContextGL::CopyTexturePlatform(ezGALTexture* pDestination, ezGALTexture* pSource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::CopyTextureRegionPlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::UpdateTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const void* pSourceData, ezUInt32 uiSourceRowPitch, ezUInt32 uiSourceDepthPitch)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::ResolveTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::ReadbackTexturePlatform(ezGALTexture* pTexture)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::CopyTextureReadbackResultPlatform(ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Debug helper functions

void ezGALContextGL::PushMarkerPlatform(const char* Marker)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::PopMarkerPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextGL::InsertEventMarkerPlatform(const char* Marker)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}