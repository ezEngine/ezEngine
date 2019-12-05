
#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

#include <UltralightPlugin/Integration/GPUDriverEz.h>

#include <UltralightPlugin/Integration/BuiltinDX11Shaders/fill_fxc.h>
#include <UltralightPlugin/Integration/BuiltinDX11Shaders/fill_path_fxc.h>
#include <UltralightPlugin/Integration/BuiltinDX11Shaders/v2f_c4f_t2f_fxc.h>
#include <UltralightPlugin/Integration/BuiltinDX11Shaders/v2f_c4f_t2f_t2f_d28f_fxc.h>

static ezUInt32 GetVertexSize(ultralight::VertexBufferFormat eFormat)
{
  switch (eFormat)
  {
    case ultralight::kVertexBufferFormat_2f_4ub_2f:
      return sizeof(ultralight::Vertex_2f_4ub_2f);
    case ultralight::kVertexBufferFormat_2f_4ub_2f_2f_28f:
      return sizeof(ultralight::Vertex_2f_4ub_2f_2f_28f);
  }

  return 0;
}


struct UltralightConstantBuffer
{
  ezVec4 State;
  ezMat4 Transform;
  ezVec4 Scalar4[2];
  ezVec4 Vector[8];
  uint32_t ClipSize;
  ezMat4 Clip[8];
};

ezUltralightGPUDriver::ezUltralightGPUDriver()
{
  EZ_LOG_BLOCK("ezUltralightGPUDriver::ezUltralightGPUDriver");

  m_pDevice = ezGALDevice::GetDefaultDevice();
  EZ_ASSERT_ALWAYS(m_pDevice != nullptr, "No GAL device created.");

  m_pContext = m_pDevice->GetPrimaryContext();
  EZ_ASSERT_ALWAYS(m_pContext != nullptr, "No GAL primary context available.");

  // Create constant buffer for the draw calls
  m_hConstantBuffer = m_pDevice->CreateConstantBuffer(sizeof(UltralightConstantBuffer));

  // Create shaders
  // TODO: This is a hack and uses direct shader code instead of proper EZ shaders

  // index 0 = ultralight::kShaderType_Fill
  {
    ezGALShaderCreationDescription shaderDesc;
    shaderDesc.m_ByteCodes[ezGALShaderStage::VertexShader] = new ezGALShaderByteCode(ezArrayPtr(v2f_c4f_t2f_t2f_d28f_fxc));
    shaderDesc.m_ByteCodes[ezGALShaderStage::PixelShader] = new ezGALShaderByteCode(ezArrayPtr(fill_fxc));

    m_hShaders[0] = m_pDevice->CreateShader(shaderDesc);
  }

  // index 1 = ultralight::kShaderType_FillPath
  {
    ezGALShaderCreationDescription shaderDesc;
    shaderDesc.m_ByteCodes[ezGALShaderStage::VertexShader] = new ezGALShaderByteCode(ezArrayPtr(v2f_c4f_t2f_fxc));
    shaderDesc.m_ByteCodes[ezGALShaderStage::PixelShader] = new ezGALShaderByteCode(ezArrayPtr(fill_path_fxc));

    m_hShaders[1] = m_pDevice->CreateShader(shaderDesc);
  }

  // Create vertex declarations for the vertex formats used

  //ultralight::kVertexBufferFormat_2f_4ub_2f
  {
    ezGALVertexDeclarationCreationDescription vertexDeclDesc;
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYFloat, 0, 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByte, offsetof(ultralight::Vertex_2f_4ub_2f, color), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat, offsetof(ultralight::Vertex_2f_4ub_2f, obj), 0, false));

    vertexDeclDesc.m_hShader = m_hShaders[1];

    m_hVertexDeclarations[0] = m_pDevice->CreateVertexDeclaration(vertexDeclDesc);
  }

  //ultralight::kVertexBufferFormat_2f_4ub_2f_2f_28f
  {
    ezGALVertexDeclarationCreationDescription vertexDeclDesc;
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYFloat, 0, 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByte, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, color), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, tex), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord1, ezGALResourceFormat::XYFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, obj), 0, false));

    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data0), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color2, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data1), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color3, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data2), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color4, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data3), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color5, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data4), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color6, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data5), 0, false));
    vertexDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color7, ezGALResourceFormat::XYZWFloat, offsetof(ultralight::Vertex_2f_4ub_2f_2f_28f, data6), 0, false));

    vertexDeclDesc.m_hShader = m_hShaders[0];

    m_hVertexDeclarations[1] = m_pDevice->CreateVertexDeclaration(vertexDeclDesc);
  }

  // Create the necessary state objects for drawing
  {
    ezGALBlendStateCreationDescription blendDesc;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = true;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend = ezGALBlend::One;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend = ezGALBlend::InvSrcAlpha;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp = ezGALBlendOp::Add;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha = ezGALBlend::InvDestAlpha;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha = ezGALBlend::One;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha = ezGALBlendOp::Add;

    m_hBlendEnabledState = m_pDevice->CreateBlendState(blendDesc);
  }

  {
    ezGALBlendStateCreationDescription blendDesc;
    blendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = false;

    m_hBlendDisabledState = m_pDevice->CreateBlendState(blendDesc);
  }

  {
    ezGALRasterizerStateCreationDescription rasterDesc;
    rasterDesc.m_bWireFrame = false;
    rasterDesc.m_CullMode = ezGALCullMode::None;
    rasterDesc.m_bFrontCounterClockwise = false;
    rasterDesc.m_iDepthBias = 0;
    rasterDesc.m_fSlopeScaledDepthBias = 0.0f;
    rasterDesc.m_fDepthBiasClamp = 0.0f;
    rasterDesc.m_bScissorTest = false;

    m_hRasterizerState = m_pDevice->CreateRasterizerState(rasterDesc);

    rasterDesc.m_bScissorTest = true;

    m_hRasterizerWithScissorTestState = m_pDevice->CreateRasterizerState(rasterDesc);
  }

  {
    ezGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = ezGALTextureFilterMode::Linear;
    samplerDesc.m_MagFilter = ezGALTextureFilterMode::Linear;
    samplerDesc.m_MipFilter = ezGALTextureFilterMode::Linear;
    samplerDesc.m_AddressU = ezImageAddressMode::Clamp;
    samplerDesc.m_AddressV = ezImageAddressMode::Clamp;
    samplerDesc.m_AddressW = ezImageAddressMode::Clamp;
    samplerDesc.m_fMinMip = 0.0f;
    samplerDesc.m_fMaxMip = 0.0f;

    m_hSamplerState = m_pDevice->CreateSamplerState(samplerDesc);

  }
}

ezUltralightGPUDriver::~ezUltralightGPUDriver()
{
  m_pDevice->DestroyBuffer(m_hConstantBuffer);

  m_pDevice->DestroyVertexDeclaration(m_hVertexDeclarations[0]);
  m_pDevice->DestroyVertexDeclaration(m_hVertexDeclarations[1]);

  m_pDevice->DestroyShader(m_hShaders[0]);
  m_pDevice->DestroyShader(m_hShaders[1]);

  m_pDevice->DestroyBlendState(m_hBlendEnabledState);
  m_pDevice->DestroyBlendState(m_hBlendDisabledState);

  m_pDevice->DestroyRasterizerState(m_hRasterizerState);
  m_pDevice->DestroyRasterizerState(m_hRasterizerWithScissorTestState);

  m_pDevice->DestroySamplerState(m_hSamplerState);

  m_pDevice = nullptr;
  m_pContext = nullptr;

  EZ_ASSERT_DEBUG(m_RenderBuffers.IsEmpty(), "Not all render buffers have been deleted");
  EZ_ASSERT_DEBUG(m_Textures.IsEmpty(), "Not all textures have been deleted");
  EZ_ASSERT_DEBUG(m_Geometries.IsEmpty(), "Not all geometries have been deleted");
}

void ezUltralightGPUDriver::BeginSynchronize()
{
  m_pContext->PushMarker("ezUltralightGPUDriver::BeginSynchronize");

  m_uiBoundRenderBufferId = 0xFFFFFFFFu;
  m_uiDrawCalls = 0;
}

void ezUltralightGPUDriver::EndSynchronize()
{
  m_pContext->PopMarker();

  ezStats::SetStat("Ultralight/NumDrawcalls", m_uiDrawCalls);
}

uint32_t ezUltralightGPUDriver::NextTextureId()
{
  return m_uiNextTextureId++;
}

void ezUltralightGPUDriver::CreateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap)
{
  if (m_Textures.Contains(texture_id))
  {
    ezLog::Error("Texture already exists.");
    return;
  }

  const ezGALResourceFormat::Enum format = bitmap->format() == ultralight::kBitmapFormat_A8_UNORM ? ezGALResourceFormat::RUByteNormalized : ezGALResourceFormat::BGRAUByteNormalizedsRGB;

  ezGALTextureCreationDescription textureDesc;
  textureDesc.m_uiArraySize = 1;
  textureDesc.m_uiMipLevelCount = 1;

  Texture texture;

  // Render target if bitmap is empty
  if (bitmap->IsEmpty())
  {
    textureDesc.SetAsRenderTarget(bitmap->width(), bitmap->height(), format);
    texture.m_hTex = m_pDevice->CreateTexture(textureDesc);
    texture.m_hRenderTargetView = m_pDevice->GetDefaultRenderTargetView(texture.m_hTex);
  }
  else
  {
    textureDesc.m_uiWidth = bitmap->width();
    textureDesc.m_uiHeight = bitmap->height();
    textureDesc.m_Format = format;

    ezGALSystemMemoryDescription initialData[1];
    initialData[0].m_pData = bitmap->LockPixels();
    initialData[0].m_uiRowPitch = bitmap->row_bytes();
    initialData[0].m_uiSlicePitch = static_cast<ezUInt32>(bitmap->size());

    texture.m_hTex = m_pDevice->CreateTexture(textureDesc, initialData);

    bitmap->UnlockPixels();
  }

  if (texture.m_hTex.IsInvalidated())
  {
    ezLog::Error("Couldn't create texture.");
    return;
  }

  texture.m_hResourceView = m_pDevice->GetDefaultResourceView(texture.m_hTex);

  ezStringBuilder builder;
  builder.Format("Ultralight Texture {0} ({1} x {2})", texture_id, bitmap->width(), bitmap->height());
  m_pDevice->GetTexture(texture.m_hTex)->SetDebugName(builder);

  m_Textures.Insert(texture_id, std::move(texture));
}

void ezUltralightGPUDriver::UpdateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap)
{
  if (auto* phTex = m_Textures.GetValue(texture_id))
  {
    ezGALTextureSubresource subresource;
    ezBoundingBoxu32 destBox;
    destBox.m_vMin.Set(0, 0, 0);
    destBox.m_vMax.Set(bitmap->width(), bitmap->height(), 1);

    ezGALSystemMemoryDescription sysmemDesc;
    sysmemDesc.m_pData = bitmap->LockPixels();
    sysmemDesc.m_uiRowPitch = bitmap->row_bytes();
    sysmemDesc.m_uiSlicePitch = static_cast<ezUInt32>(bitmap->size());   

    m_pContext->UpdateTexture(phTex->m_hTex, subresource, destBox, sysmemDesc);

    bitmap->UnlockPixels();
  }
  else
  {
    ezLog::Error("Trying to update non existing texture");
  }
}

void ezUltralightGPUDriver::BindTexture(uint8_t texture_unit, uint32_t texture_id)
{
  if (auto* phTex = m_Textures.GetValue(texture_id))
  {
    m_pContext->SetResourceView(ezGALShaderStage::PixelShader, texture_unit, phTex->m_hResourceView);
  }
}

void ezUltralightGPUDriver::DestroyTexture(uint32_t texture_id)
{
  if (auto* phTex = m_Textures.GetValue(texture_id))
  {
    m_pDevice->DestroyTexture(phTex->m_hTex);

    m_Textures.Remove(texture_id);
  }
}

ezGALTextureHandle ezUltralightGPUDriver::GetTextureHandleForTextureId(uint32_t texture_id)
{
  if (auto* phTex = m_Textures.GetValue(texture_id))
  {
    return phTex->m_hTex;
  }
  
  return ezGALTextureHandle();
}

uint32_t ezUltralightGPUDriver::NextRenderBufferId()
{
  return m_uiNextRenderBufferId++;
}

void ezUltralightGPUDriver::CreateRenderBuffer(uint32_t render_buffer_id, const ultralight::RenderBuffer& buffer)
{
  if (m_RenderBuffers.Contains(render_buffer_id))
  {
    ezLog::Error("Render buffer already exists.");
    return;
  }

  EZ_ASSERT_DEV(!buffer.has_depth_buffer, "Ultralight specifies depth buffer but not supported at the moment.");
  EZ_ASSERT_DEV(!buffer.has_stencil_buffer, "Ultralight specifies stencil buffer but not supported at the moment.");

  if (auto* phTex = m_Textures.GetValue(buffer.texture_id))
  {
    RenderBuffer renderBuffer;
    renderBuffer.m_hRenderTarget = phTex->m_hTex;
    renderBuffer.m_hRenderTargetView = phTex->m_hRenderTargetView;

    // TODO: D3D11 driver implementation doesn't cater to depth/stencil
    // so we don't either
    renderBuffer.m_hDepthBuffer.Invalidate();
    renderBuffer.m_hDepthStencilView.Invalidate();

    m_RenderBuffers.Insert(render_buffer_id, std::move(renderBuffer));
  }  
}

void ezUltralightGPUDriver::BindRenderBuffer(uint32_t render_buffer_id)
{
  if (m_uiBoundRenderBufferId == render_buffer_id)
    return;

  if (auto* pRenderBuffer = m_RenderBuffers.GetValue(render_buffer_id))
  {
    ezGALRenderTargetSetup setup;
    setup.SetRenderTarget(0, pRenderBuffer->m_hRenderTargetView);
    setup.SetDepthStencilTarget(pRenderBuffer->m_hDepthStencilView);

    m_pContext->SetRenderTargetSetup(setup);

    m_uiBoundRenderBufferId = render_buffer_id;
  }
}

void ezUltralightGPUDriver::ClearRenderBuffer(uint32_t render_buffer_id)
{
  BindRenderBuffer(render_buffer_id);

  // No depth/stencil clear as the D3D11 base implementation also doesn't
  // support it
  m_pContext->Clear(ezColor::Black.WithAlpha(0), 0xFF, false, false);
}

void ezUltralightGPUDriver::DestroyRenderBuffer(uint32_t render_buffer_id)
{
  m_RenderBuffers.Remove(render_buffer_id);
}

uint32_t ezUltralightGPUDriver::NextGeometryId()
{
  return m_uiNextGeometryId++;
}

void ezUltralightGPUDriver::CreateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices)
{
  if (m_Geometries.Contains(geometry_id))
  {
    ezLog::Error("Geometry already exists.");
    return;
  }

  Geometry geometry;

  {
    ezArrayPtr<ezUInt8> initialVBData;
    if (vertices.data != nullptr)
    {
      initialVBData = ezArrayPtr(vertices.data, vertices.size);
    }

    ezGALBufferCreationDescription bufferDesc;
    bufferDesc.m_BufferType = ezGALBufferType::VertexBuffer;
    bufferDesc.m_uiStructSize = GetVertexSize(vertices.format);
    bufferDesc.m_uiTotalSize = vertices.size;
    bufferDesc.m_ResourceAccess.m_bImmutable = false;

    geometry.m_hVertexBuffer = m_pDevice->CreateBuffer(bufferDesc, initialVBData);
  }

  {
    ezArrayPtr<ezUInt8> initialIBData;
    if (indices.data)
    {
      initialIBData = ezArrayPtr(indices.data, indices.size);
    }

    ezGALBufferCreationDescription bufferDesc;
    bufferDesc.m_BufferType = ezGALBufferType::IndexBuffer;
    bufferDesc.m_uiStructSize = sizeof(ezUInt32);
    bufferDesc.m_uiTotalSize = indices.size;
    bufferDesc.m_ResourceAccess.m_bImmutable = false;

    geometry.m_hIndexBuffer = m_pDevice->CreateBuffer(bufferDesc, initialIBData);
  }

  geometry.m_eVertexFormat = vertices.format;

  m_Geometries.Insert(geometry_id, std::move(geometry));
}

void ezUltralightGPUDriver::UpdateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices)
{
  if (auto* pGeometry = m_Geometries.GetValue(geometry_id))
  {
    m_pContext->UpdateBuffer(pGeometry->m_hVertexBuffer, 0, ezArrayPtr(vertices.data, vertices.size));
    m_pContext->UpdateBuffer(pGeometry->m_hIndexBuffer, 0, ezArrayPtr(indices.data, indices.size));
  }
}

void ezUltralightGPUDriver::DrawGeometry(uint32_t geometry_id, uint32_t indices_count, uint32_t indices_offset, const ultralight::GPUState& state)
{
  if (auto* pGeometry = m_Geometries.GetValue(geometry_id))
  {
    BindRenderBuffer(state.render_buffer_id);

    // Update constant buffer
    UltralightConstantBuffer constantBuffer;
    constantBuffer.State.Set(0, state.viewport_width, state.viewport_height, 1.0f /* context_->scale() */);
    constantBuffer.Transform.SetFromArray(state.transform.data, ezMatrixLayout::ColumnMajor);
    constantBuffer.Scalar4[0].Set(state.uniform_scalar[0], state.uniform_scalar[1], state.uniform_scalar[2], state.uniform_scalar[3]);
    constantBuffer.Scalar4[1].Set(state.uniform_scalar[5], state.uniform_scalar[5], state.uniform_scalar[6], state.uniform_scalar[7]);
    for (int i = 0; i < 8; ++i)
    {
      constantBuffer.Vector[i].Set(state.uniform_vector[i].x, state.uniform_vector[i].y, state.uniform_vector[i].z, state.uniform_vector[i].w);
    }

    constantBuffer.ClipSize = state.clip_size;
    for (uint8_t i = 0; i < state.clip_size; ++i)
    {
      constantBuffer.Clip[i].SetFromArray(state.clip[i].data, ezMatrixLayout::ColumnMajor);
    }

    m_pContext->UpdateBuffer(m_hConstantBuffer, 0, ezArrayPtr(reinterpret_cast<const ezUInt8*>(&constantBuffer), sizeof(constantBuffer)));
    m_pContext->SetConstantBuffer(0, m_hConstantBuffer);

    m_pContext->SetViewport(ezRectFloat(state.viewport_width, state.viewport_height));

    m_pContext->SetSamplerState(ezGALShaderStage::PixelShader, 0, m_hSamplerState);

    if (state.texture_1_id)
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 0, m_Textures.GetValue(state.texture_1_id)->m_hResourceView);
    }
    else
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 0, ezGALResourceViewHandle());
    }

    if (state.texture_2_id)
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 1, m_Textures.GetValue(state.texture_2_id)->m_hResourceView);
    }
    else
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 1, ezGALResourceViewHandle());
    }

    if (state.texture_3_id)
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 2, m_Textures.GetValue(state.texture_3_id)->m_hResourceView);
    }
    else
    {
      m_pContext->SetResourceView(ezGALShaderStage::PixelShader, 2, ezGALResourceViewHandle());
    }

    auto* pGeom = m_Geometries.GetValue(geometry_id);

    m_pContext->SetIndexBuffer(pGeom->m_hIndexBuffer);
    m_pContext->SetVertexBuffer(0, pGeom->m_hVertexBuffer);

    m_pContext->SetPrimitiveTopology(ezGALPrimitiveTopology::Triangles);
    m_pContext->SetVertexDeclaration(pGeom->m_eVertexFormat == ultralight::kVertexBufferFormat_2f_4ub_2f ? m_hVertexDeclarations[0] : m_hVertexDeclarations[1]);

    m_pContext->SetShader(state.shader_type == ultralight::kShaderType_Fill ? m_hShaders[0] : m_hShaders[1]);

    m_pContext->SetBlendState(state.enable_blend ? m_hBlendEnabledState : m_hBlendDisabledState);

    if (state.enable_scissor)
    {
      m_pContext->SetRasterizerState(m_hRasterizerWithScissorTestState);
      m_pContext->SetScissorRect(
        ezRectU32(
          static_cast<ezUInt32>(state.scissor_rect.left),
          static_cast<ezUInt32>(state.scissor_rect.top),
          static_cast<ezUInt32>(state.scissor_rect.width()),
          static_cast<ezUInt32>(state.scissor_rect.height())
            )
        );
    }
    else
    {
      m_pContext->SetRasterizerState(m_hRasterizerState);
    }

    m_pContext->DrawIndexed(indices_count, indices_offset);
  }
}

void ezUltralightGPUDriver::DestroyGeometry(uint32_t geometry_id)
{
  if (auto* pGeometry = m_Geometries.GetValue(geometry_id))
  {
    m_pDevice->DestroyBuffer(pGeometry->m_hVertexBuffer);
    m_pDevice->DestroyBuffer(pGeometry->m_hIndexBuffer);

    m_Geometries.Remove(geometry_id);
  }
}

void ezUltralightGPUDriver::UpdateCommandList(const ultralight::CommandList& list)
{
  if (list.size > 0)
  {
    m_Commands.SetCountUninitialized(list.size);
    ezMemoryUtils::Copy(&m_Commands[0], list.commands, list.size);
  }
}

bool ezUltralightGPUDriver::HasCommandsPending()
{
  return !m_Commands.IsEmpty();
}

void ezUltralightGPUDriver::DrawCommandList()
{
  EZ_PROFILE_SCOPE("ezUltralightGPUDriver::DrawCommandList");

  m_pContext->PushMarker("ezUltralightGPUDriver::DrawCommandList");

  for (const auto& command : m_Commands)
  {
    switch (command.command_type)
    {
    case ultralight::kCommandType_ClearRenderBuffer:
      ClearRenderBuffer(command.gpu_state.render_buffer_id);
      break;

    case ultralight::kCommandType_DrawGeometry:
      DrawGeometry(command.geometry_id, command.indices_count, command.indices_offset, command.gpu_state);
      break;
    }
  }

  m_pContext->PopMarker();

  m_Commands.Clear();
}
