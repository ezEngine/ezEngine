#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Blurs input and writes it to an output buffer of the same format.
class EZ_RENDERERCORE_DLL ezBlurPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBlurPass, ezRenderPipelinePass);

public:
  ezBlurPass();
  ~ezBlurPass();

  virtual bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
    ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(ezInt32 iRadius);
  ezInt32 GetRadius() const;

protected:
  ezInputNodePin m_PinInput;
  ezOutputNodePin m_PinOutput;

  ezInt32 m_iRadius;
  ezConstantBufferStorageHandle m_hBlurCB;
  ezShaderResourceHandle m_hShader;
};

