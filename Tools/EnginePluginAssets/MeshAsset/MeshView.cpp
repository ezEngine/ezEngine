#include <PCH.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>

ezMeshViewContext::ezMeshViewContext(ezMeshContext* pMeshContext) 
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pMeshContext = pMeshContext;
  m_pView = nullptr;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezMeshViewContext::~ezMeshViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

bool ezMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, ezVec3(1.0f, 1.0f, -1.0f));
}


ezView* ezMeshViewContext::CreateView()
{
  ezView* pView = ezRenderLoop::CreateView("Mesh Editor - View");

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetLogicCamera(&m_Camera);
  return pView;
}
