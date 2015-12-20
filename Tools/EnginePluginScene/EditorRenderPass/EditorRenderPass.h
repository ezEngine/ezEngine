#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezEditorRenderPass : public ezSimpleRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorRenderPass, ezSimpleRenderPass);
public:
  ezEditorRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName = "EditorRenderPass");

  virtual void Execute(const ezRenderViewContext& renderViewContext) override;

  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  ezViewRenderMode::Enum m_ViewRenderMode;
};
