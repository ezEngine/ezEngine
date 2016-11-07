#include <PCH.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>
#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>

ezTextureViewContext::ezTextureViewContext(ezTextureContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
  m_pTextureContext = pContext;
  m_pView = nullptr;
}

ezTextureViewContext::~ezTextureViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

ezView* ezTextureViewContext::CreateView()
{
  ezView* pView = ezRenderLoop::CreateView("Texture Editor - View");

  pView->SetRenderPipelineResource(CreateDebugRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetLogicCamera(&m_Camera);
  return pView;
}

void ezTextureViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  // Do not apply render mode here otherwise we would switch to a different pipeline.
  // Also use hardcoded clipping planes so the quad is not culled to early.

  ezCameraMode::Enum cameraMode = (ezCameraMode::Enum)pMsg->m_iCameraMode;
  m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, 0.0001f, 50.0f);
  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);
}
