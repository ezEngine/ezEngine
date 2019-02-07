#include <EnginePluginKrautPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezKrautTreeViewContext::ezKrautTreeViewContext(ezKrautTreeContext* pKrautTreeContext)
    : ezEngineProcessViewContext(pKrautTreeContext)
{
  m_pKrautTreeContext = pKrautTreeContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezKrautTreeViewContext::~ezKrautTreeViewContext() {}

bool ezKrautTreeViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}

ezViewHandle ezKrautTreeViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Kraut Tree Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(ezCameraUsageHint::Thumbnail);
  return pView->GetHandle();
}

void ezKrautTreeViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  ezBoundingBox bbox;
  bbox.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3::ZeroVector());

  auto hResource = m_pKrautTreeContext->GetResource();
  if (hResource.IsValid())
  {
    ezResourceLock<ezKrautTreeResource> pResource(hResource, ezResourceAcquireMode::AllowFallback);

    if (pResource->GetDetails().m_Bounds.IsValid())
    {
      bbox = pResource->GetDetails().m_Bounds.GetBox();

      ezStringBuilder sText;
      sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
                          ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

      ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 26), ezColor::White);
    }
  }
}
