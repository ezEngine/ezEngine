#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
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
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>

ezEngineProcessViewContext::ezEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
  : m_pDocumentContext(pContext), m_pView(nullptr)
{
  
}

ezEngineProcessViewContext::~ezEngineProcessViewContext()
{

}

void ezEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
      HandleWindowUpdate(reinterpret_cast<HWND>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
      Redraw(true);
    }
  }
}

void ezEngineProcessViewContext::SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void ezEngineProcessViewContext::HandleWindowUpdate(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezEngineProcessViewContext::HandleWindowUpdate");

  if (GetEditorWindow().m_hWnd != 0)
  {
    if (GetEditorWindow().m_uiWidth == uiWidth && GetEditorWindow().m_uiHeight == uiHeight)
      return;

    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  GetEditorWindow().m_hWnd = hWnd;
  GetEditorWindow().m_uiWidth = uiWidth;
  GetEditorWindow().m_uiHeight = uiHeight;

  ezLog::Debug("Creating Swapchain with size %u * %u", uiWidth, uiHeight);

  auto hPrimarySwapChain = static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->AddWindow(&GetEditorWindow());
  const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(hPrimarySwapChain);
  EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

  auto hSwapChainRTV = pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());
  auto hSwapChainDSV = pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetDepthStencilBufferTexture());

  ezGALRenderTagetSetup BackBufferRenderTargetSetup;
  BackBufferRenderTargetSetup.SetRenderTarget(0, hSwapChainRTV).SetDepthStencilTarget(hSwapChainDSV);

  SetupRenderTarget(BackBufferRenderTargetSetup, uiWidth, uiHeight);
}

void ezEngineProcessViewContext::SetupRenderTarget(ezGALRenderTagetSetup& renderTargetSetup, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezEngineProcessViewContext::SetupRenderTarget");

  // setup view
  {
    if (m_pView == nullptr)
    {
      m_pView = CreateView();
    }

    m_pView->SetRenderTargetSetup(renderTargetSetup);
    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
  }
}

void ezEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  auto pState = ezGameApplication::GetGameApplicationInstance()->GetGameStateForWorld(GetDocumentContext()->GetWorld());

  if (pState != nullptr)
  {
    pState->AddAllMainViews();
  }

  // setting to only update one view ?
  //else
  {
    ezTag tagEditor;
    ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor", &tagEditor);

    if (!bRenderEditorGizmos)
    {
      // exclude all editor objects from rendering in proper game views
      m_pView->m_ExcludeTags.Set(tagEditor);
    }
    else
    {
      m_pView->m_ExcludeTags.Remove(tagEditor);
    }

    ezRenderLoop::AddMainView(m_pView);
  }
}

bool ezEngineProcessViewContext::FocusCameraOnObject(ezCamera& camera, const ezBoundingBoxSphere& objectBounds, float fFov, const ezVec3& vViewDir)
{
  ezVec3 vDir = vViewDir;
  bool bChanged = false;
  ezVec3 vCameraPos = camera.GetCenterPosition();
  ezVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = objectBounds.GetSphere().m_fRadius / ezMath::Sin(ezAngle::Degree(fFov / 2));
  vDir.Normalize();
  ezVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    camera.LookAt(vNewCameraPos, vCenterPos, ezVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void ezEngineProcessViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pView)
    m_pView->SetCameraUsageHint(pMsg->m_CameraUsageHint);
  m_Camera.SetCameraMode((ezCameraMode::Enum) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  if (m_pView)
  {
    ezRenderPipelineResourceHandle hRenderPipeline = m_pView->GetRenderPipelineResource();
    ezResourceLock<ezRenderPipelineResource> pPipeline(hRenderPipeline, ezResourceAcquireMode::NoFallback);
    if (pMsg->m_sRenderPipelineResource.IsEmpty() && !IsDefaultRenderPipeline(hRenderPipeline))
    {
      auto hPipe = CreateDefaultRenderPipeline();
      if (hPipe.IsValid())
      {
        ezLog::Info("Setting view's render pipeline to: '%s'", "<Default>");
        m_pView->SetRenderPipelineResource(hPipe);
      }
    }
    else if (!pMsg->m_sRenderPipelineResource.IsEmpty() && pPipeline->GetResourceID() != pMsg->m_sRenderPipelineResource)
    {
      auto hPipe = ezResourceManager::LoadResource<ezRenderPipelineResource>(pMsg->m_sRenderPipelineResource);
      if (hPipe.IsValid())
      {
        ezLog::Info("Setting view's render pipeline to: '%s'", pMsg->m_sRenderPipelineResource.GetData());
        m_pView->SetRenderPipelineResource(hPipe);
      }
    }
  }
}

bool ezEngineProcessViewContext::IsDefaultRenderPipeline(ezRenderPipelineResourceHandle hPipeline)
{
  return hPipeline == ezGameState::GetMainRenderPipeline();
}

ezRenderPipelineResourceHandle ezEngineProcessViewContext::CreateDefaultRenderPipeline()
{
  return ezGameState::GetMainRenderPipeline();
}
