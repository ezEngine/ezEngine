#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Screen-space contact shadows using per-pixel ray marching.
///
/// This implementation is using the "screen space shadow" code by Bend studio.
class EZ_RENDERERCORE_DLL ezScreenSpaceShadowPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScreenSpaceShadowPass, ezRenderPipelinePass);

public:
  ezScreenSpaceShadowPass();
  ~ezScreenSpaceShadowPass();

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;
  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

protected:
  void CreateSamplerState();

  ezRenderPipelineNodeInputPin m_PinDepthInput;
  ezRenderPipelineNodeOutputPin m_PinOutput;

  ezConstantBufferStorageHandle m_hConstantBuffer;
  ezGALSamplerStateHandle m_hDepthSamplerState;

  ezShaderResourceHandle m_hShader;

  float m_fSurfaceThickness = 0.005f;
  float m_fShadowContrast = 4.0f;
};
