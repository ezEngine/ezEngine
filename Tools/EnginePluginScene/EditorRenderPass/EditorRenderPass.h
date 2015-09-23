#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezEditorRenderPass : public ezSimpleRenderPass
{
public:
  ezEditorRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName = "EditorRenderPass");

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

  ezViewRenderMode::Enum m_ViewRenderMode;
};
