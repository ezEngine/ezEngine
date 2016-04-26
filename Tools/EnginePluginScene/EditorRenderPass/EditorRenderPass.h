#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezSceneContext;

class ezEditorRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorRenderPass, ezForwardRenderPass);
public:
  ezEditorRenderPass(const char* szName = "EditorRenderPass");

  virtual void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
    const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  ezViewRenderMode::Enum m_ViewRenderMode;
};
