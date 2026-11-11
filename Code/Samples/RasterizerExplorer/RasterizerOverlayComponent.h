#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// Render data that carries a GAL texture handle to the renderer.
class ezRasterizerOverlayRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRasterizerOverlayRenderData, ezRenderData);

public:
  ezGALTextureHandle m_hTexture;
};

using ezRasterizerOverlayComponentManager = ezComponentManager<class ezRasterizerOverlayComponent, ezBlockStorageType::FreeList>;

/// Renders a fullscreen overlay of the software rasterizer output.
///
/// Attach to any game object. When the texture handle is set and valid, it is drawn as
/// a fullscreen quad on top of the scene. Black pixels are discarded so that debug geometry shows through.
class ezRasterizerOverlayComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRasterizerOverlayComponent, ezRenderComponent, ezRasterizerOverlayComponentManager);

public:
  ezRasterizerOverlayComponent();
  ~ezRasterizerOverlayComponent();

  void SetTextureHandle(ezGALTextureHandle hTexture) { m_hTexture = hTexture; }
  ezGALTextureHandle GetTextureHandle() const { return m_hTexture; }

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override {}

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezGALTextureHandle m_hTexture;
};

/// Renders batches of ezRasterizerOverlayRenderData as fullscreen quads.
class ezRasterizerOverlayRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRasterizerOverlayRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerOverlayRenderer);

public:
  ezRasterizerOverlayRenderer();
  ~ezRasterizerOverlayRenderer();

  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

private:
  ezShaderResourceHandle m_hShader;
};
