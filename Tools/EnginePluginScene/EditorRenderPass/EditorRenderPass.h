#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

class ezSceneContext;

class ezEditorRenderPass : public ezForwardRenderPass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorRenderPass, ezForwardRenderPass);
public:
  ezEditorRenderPass(const char* szName = "EditorRenderPass");

protected:
  virtual void SetupPermutationVars(const ezRenderViewContext& renderViewContext) override;

  ezViewRenderMode::Enum m_ViewRenderMode;
};
