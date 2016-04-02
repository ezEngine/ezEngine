#include <PCH.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <GameFoundation/GameApplication/GameApplication.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/Component.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <SceneContext/SceneContext.h>
#include <RenderPipeline/EditorSelectedObjectsExtractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>

const char* s_szEditorPipelineID = "EditorRenderPipeline";

ezSceneViewContext::ezSceneViewContext(ezSceneContext* pSceneContext) : ezEngineProcessViewContext(pSceneContext)
{
  m_pSceneContext = pSceneContext;
  m_pView = nullptr;
  m_bUpdatePickingData = true;
}

ezSceneViewContext::~ezSceneViewContext()
{
  ezRenderLoop::DeleteView(m_pView);

  if (GetEditorWindow().m_hWnd != 0)
  {
    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(&GetEditorWindow());
  }
}

void ezSceneViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezViewContext::SetupRenderTarget");

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

  // setup view
  {
    if (m_pView == nullptr)
    {
      CreateView();
    }

    ezGALRenderTagetSetup BackBufferRenderTargetSetup;
    BackBufferRenderTargetSetup.SetRenderTarget(0, hSwapChainRTV).SetDepthStencilTarget(hSwapChainDSV);
    m_pView->SetRenderTargetSetup(BackBufferRenderTargetSetup);
    m_pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
  }
}

void ezSceneViewContext::Redraw()
{
  ezTag tagNoOrtho;
  ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode", &tagNoOrtho);

  if (m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedHeight ||
      m_pView->GetRenderCamera()->GetCameraMode() == ezCameraMode::OrthoFixedWidth)
  {
    m_pView->m_ExcludeTags.Set(tagNoOrtho);
  }
  else
  {
    m_pView->m_ExcludeTags.Remove(tagNoOrtho);
  }

  auto pState = ezGameApplication::GetGameApplicationInstance()->GetGameStateForWorld(m_pSceneContext->GetWorld());

  if (pState != nullptr)
  {
    pState->AddAllMainViews();
  }

  // setting to only update one view ?
  //else
  {
    ezRenderLoop::AddMainView(m_pView);
  }
}

void ezSceneViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  m_Camera.SetCameraMode((ezCameraMode::Enum) pMsg->m_iCameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);

  m_Camera.LookAt(pMsg->m_vPosition, pMsg->m_vPosition + pMsg->m_vDirForwards, pMsg->m_vDirUp);

  if (m_pView)
  {
    ezRenderPipelineResourceHandle hRenderPipeline = m_pView->GetRenderPipelineResource();
    ezResourceLock<ezRenderPipelineResource> pPipeline(hRenderPipeline, ezResourceAcquireMode::NoFallback);
    if (pMsg->m_sRenderPipelineResource.IsEmpty() && pPipeline->GetResourceID() != s_szEditorPipelineID)
    {
      auto hPipe = CreateEditorRenderPipeline();
      if (hPipe.IsValid())
      {
        ezLog::Info("Setting view's render pipeline to: '%s'", s_szEditorPipelineID);
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

    m_pView->SetRenderPassProperty("EditorRenderPass", "RenderSelectionOverlay", m_pSceneContext->GetRenderSelectionOverlay());
    m_pView->SetRenderPassProperty("EditorRenderPass", "ViewRenderMode", pMsg->m_uiRenderMode);
    m_pView->SetRenderPassProperty("EditorPickingPass", "ViewRenderMode", pMsg->m_uiRenderMode);
  }
}

void ezSceneViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;

  if (m_pView != nullptr && m_pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickingMatrix"))
  {
    m_pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", ezVec2(x, y));
    ezVariant varMat = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingMatrix");

    if (varMat.IsA<ezMat4>())
    {
      const ezMat4 mInverseMat = varMat.Get<ezMat4>();
      const ezUInt32 uiPickingID = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingID").ConvertTo<ezUInt32>();
      const float fPickingDepth = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingDepth").ConvertTo<float>();
      res.m_vPickedNormal = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingNormal").ConvertTo<ezVec3>();
      res.m_vPickingRayStartPosition = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingRayStartPosition").ConvertTo<ezVec3>();
      res.m_vPickedPosition = m_pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickingPosition").ConvertTo<ezVec3>();

      EZ_ASSERT_DEBUG(!res.m_vPickedPosition.IsNaN(), "")
        ;
      const ezUInt32 uiComponentID = (uiPickingID & 0x00FFFFFF);
      const ezUInt32 uiPartIndex = (uiPickingID >> 24);

      res.m_ComponentGuid = GetDocumentContext()->m_Context.m_ComponentPickingMap.GetGuid(uiComponentID);
      res.m_OtherGuid = GetDocumentContext()->m_Context.m_OtherPickingMap.GetGuid(uiComponentID);

      if (res.m_ComponentGuid.IsValid())
      {
        ezComponentHandle hComponent = GetDocumentContext()->m_Context.m_ComponentMap.GetHandle(res.m_ComponentGuid);

        ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();

        // check whether the component is still valid
        ezComponent* pComponent = nullptr;
        if (pDocumentContext->GetWorld()->TryGetComponent<ezComponent>(hComponent, pComponent))
        {
          // if yes, fill out the parent game object guid
          res.m_ObjectGuid = GetDocumentContext()->m_Context.m_GameObjectMap.GetGuid(pComponent->GetOwner()->GetHandle());
          res.m_uiPartIndex = uiPartIndex;
        }
        else
        {
          res.m_ComponentGuid = ezUuid();
        }
      }
    }
  }

  SendViewMessage(&res);
}

void ezSceneViewContext::SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void ezSceneViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    if (m_pView)
    {
      m_pView->SetRenderPassProperty("EditorPickingPass", "Enable", pMsg2->m_bUpdatePickingData);
      m_pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
    }

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
      SetupRenderTarget(reinterpret_cast<HWND>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
      Redraw();
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {


    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }


}

void ezSceneViewContext::CreateView()
{
  m_pView = ezRenderLoop::CreateView("Editor - View");

  m_pView->SetRenderPipelineResource(CreateEditorRenderPipeline());


  m_pView->SetRenderPassProperty("EditorRenderPass", "SceneContext", m_pSceneContext);
  m_pView->SetRenderPassProperty("EditorPickingPass", "SceneContext", m_pSceneContext);
  m_pView->SetExtractorProperty("EditorSelectedObjectsExtractor", "SceneContext", m_pSceneContext);

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  m_pView->SetWorld(pDocumentContext->GetWorld());
  m_pView->SetLogicCamera(&m_Camera);

  auto& tagReg = ezTagRegistry::GetGlobalRegistry();
  ezTag tagHidden;
  tagReg.RegisterTag("EditorHidden", &tagHidden);

  m_pView->m_ExcludeTags.Set(tagHidden);
}

ezRenderPipelineResourceHandle ezSceneViewContext::CreateEditorRenderPipeline()
{
  auto hPipe = ezResourceManager::GetExistingResource<ezRenderPipelineResource>(s_szEditorPipelineID);
  if (hPipe.IsValid())
  {
    return hPipe;
  }

  ezUniquePtr<ezRenderPipeline> pRenderPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezEditorRenderPass* pEditorRenderPass = nullptr;
  {
    ezUniquePtr<ezEditorRenderPass> pPass = EZ_DEFAULT_NEW(ezEditorRenderPass);
    pEditorRenderPass = pPass.Borrow();
    pEditorRenderPass->SetName("EditorRenderPass");
    pEditorRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pEditorRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezLightGatheringRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezPickingRenderPass* pPickingRenderPass = nullptr;
  {
    ezUniquePtr<ezPickingRenderPass> pPass = EZ_DEFAULT_NEW(ezPickingRenderPass);
    pPickingRenderPass = pPass.Borrow();
    pPickingRenderPass->SetName("EditorPickingPass");
    pPickingRenderPass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezSimpleRenderPass* pSimplePass = nullptr;
  {
    ezUniquePtr<ezSimpleRenderPass> pPass = EZ_DEFAULT_NEW(ezSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pSimplePass->AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
    pRenderPipeline->AddPass(std::move(pPass));
  }

  ezTargetPass* pTargetPass = nullptr;
  {
    ezUniquePtr<ezTargetPass> pPass = EZ_DEFAULT_NEW(ezTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  EZ_VERIFY(pRenderPipeline->Connect(pEditorRenderPass, "Color", pSimplePass, "Color"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pEditorRenderPass, "DepthStencil", pSimplePass, "DepthStencil"), "Connect failed!");

  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");
  EZ_VERIFY(pRenderPipeline->Connect(pSimplePass, "DepthStencil", pTargetPass, "DepthStencil"), "Connect failed!");

  //m_pPickingRenderPass->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneViewContext::RenderPassEventHandler, this));

  ezUniquePtr<ezEditorSelectedObjectsExtractor> pExtractorSelection = EZ_DEFAULT_NEW(ezEditorSelectedObjectsExtractor);
  pExtractorSelection->SetName("EditorSelectedObjectsExtractor");

  //ezUniquePtr<ezCallDelegateExtractor> pExtractorShapeIcons = EZ_DEFAULT_NEW(ezCallDelegateExtractor);
  //pExtractorShapeIcons->m_Delegate = ezMakeDelegate(&ezSceneContext::GenerateShapeIconMesh, m_pSceneContext);

  pRenderPipeline->AddExtractor(std::move(pExtractorSelection));
  //pRenderPipeline->AddExtractor(std::move(pExtractorShapeIcons));
  pRenderPipeline->AddExtractor(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));

  ezRenderPipelineResourceDescriptor desc;
  ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return ezResourceManager::CreateResource<ezRenderPipelineResource>(s_szEditorPipelineID, desc);

}
