
#pragma once

#include <Ultralight/Ultralight.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/DynamicArray.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/Buffer.h>

class ezGALDevice;
class ezGALContext;

EZ_DEFINE_AS_POD_TYPE(ultralight::Command);

class ezUltralightGPUDriver : public ultralight::GPUDriver
{
public:

  ezUltralightGPUDriver();
  ~ezUltralightGPUDriver();

  virtual void BeginSynchronize() override;
  virtual void EndSynchronize() override;

  virtual uint32_t NextTextureId() override;

  virtual void CreateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap) override;
  virtual void UpdateTexture(uint32_t texture_id, ultralight::Ref<ultralight::Bitmap> bitmap) override;
  virtual void BindTexture(uint8_t texture_unit, uint32_t texture_id) override;
  virtual void DestroyTexture(uint32_t texture_id) override;
  ezGALTextureHandle GetTextureHandleForTextureId(uint32_t texture_id);

  virtual uint32_t NextRenderBufferId() override;

  virtual void CreateRenderBuffer(uint32_t render_buffer_id, const ultralight::RenderBuffer& buffer) override;
  virtual void BindRenderBuffer(uint32_t render_buffer_id) override;
  virtual void ClearRenderBuffer(uint32_t render_buffer_id) override;
  virtual void DestroyRenderBuffer(uint32_t render_buffer_id) override;

  virtual uint32_t NextGeometryId() override;

  virtual void CreateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices) override;
  virtual void UpdateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer& vertices, const ultralight::IndexBuffer& indices) override;
  virtual void DrawGeometry(uint32_t geometry_id, uint32_t indices_count, uint32_t indices_offset, const ultralight::GPUState& state) override;
  virtual void DestroyGeometry(uint32_t geometry_id) override;

  virtual void UpdateCommandList(const ultralight::CommandList& list) override;
  virtual bool HasCommandsPending() override;
  virtual void DrawCommandList() override;

protected:

  struct Texture
  {
    ezGALTextureHandle m_hTex;
    ezGALResourceViewHandle m_hResourceView;
    ezGALRenderTargetViewHandle m_hRenderTargetView;
  };

  struct Geometry
  {
    ezGALBufferHandle m_hVertexBuffer;
    ultralight::VertexBufferFormat m_eVertexFormat;

    ezGALBufferHandle m_hIndexBuffer;
  };

  struct RenderBuffer
  {
    ezGALTextureHandle m_hRenderTarget;
    ezGALRenderTargetViewHandle m_hRenderTargetView;

    ezGALTextureHandle m_hDepthBuffer;
    ezGALRenderTargetViewHandle m_hDepthStencilView;
  };

  ezMap<uint32_t, Texture> m_Textures;
  uint32_t m_uiNextTextureId = 1;

  ezMap<uint32_t, RenderBuffer> m_RenderBuffers;
  uint32_t m_uiNextRenderBufferId = 1;
  uint32_t m_uiBoundRenderBufferId = 0xFFFFFFFFu;

  ezMap<uint32_t, Geometry> m_Geometries;
  uint32_t m_uiNextGeometryId = 1;

  ezDynamicArray<ultralight::Command> m_Commands;

  ezGALBufferHandle m_hConstantBuffer;

  // index 0 = ultralight::kShaderType_Fill
  // index 1 = ultralight::kShaderType_FillPath
  ezGALShaderHandle m_hShaders[2];

  ezGALBlendStateHandle m_hBlendEnabledState;
  ezGALBlendStateHandle m_hBlendDisabledState;

  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALRasterizerStateHandle m_hRasterizerWithScissorTestState;

  ezGALSamplerStateHandle m_hSamplerState;

  // index 0 = ultralight::kVertexBufferFormat_2f_4ub_2f
  // index 1 = ultralight::kVertexBufferFormat_2f_4ub_2f_2f_28f
  ezGALVertexDeclarationHandle m_hVertexDeclarations[2];

  ezGALDevice* m_pDevice = nullptr;
  ezGALContext* m_pContext = nullptr;

  ezUInt32 m_uiDrawCalls = 0;
};
