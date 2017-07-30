#include <PCH.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Logging/Log.h>
#include <Gizmos/GizmoHandle.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Image/ImageUtils.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineProcessDocumentContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;

ezEngineProcessDocumentContext* ezEngineProcessDocumentContext::GetDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void ezEngineProcessDocumentContext::AddDocumentContext(ezUuid guid, ezEngineProcessDocumentContext* pContext, ezEngineProcessCommunicationChannel* pIPC)
{
  EZ_ASSERT_DEV(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
  s_DocumentContexts[guid] = pContext;

  pContext->Initialize(guid, pIPC);
}

bool ezEngineProcessDocumentContext::PendingOperationsInProgress()
{
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->PendingOperationInProgress())
      return true;
  }
  return false;
}

void ezEngineProcessDocumentContext::UpdateDocumentContexts()
{
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->UpdateDocumentContext();
  }
}

void ezEngineProcessDocumentContext::DestroyDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pContext = nullptr;
  if (s_DocumentContexts.Remove(guid, &pContext))
  {
    pContext->Deinitialize();
    pContext->GetDynamicRTTI()->GetAllocator()->Deallocate(pContext);
  }
}

ezBoundingBoxSphere ezEngineProcessDocumentContext::GetWorldBounds(ezWorld* pWorld)
{
  ezBoundingBoxSphere bounds;
  bounds.SetInvalid();

  {
    EZ_LOCK(pWorld->GetReadMarker());

    EZ_ASSERT_DEV(!pWorld->GetWorldSimulationEnabled(), "World simulation must be disabled to get bounds!");

    const ezWorld* pConstWorld = pWorld;
    for (auto it = pConstWorld->GetObjects(); it.IsValid(); ++it)
    {
      const ezGameObject* pObj = it;

      const auto& b = pObj->GetGlobalBounds();

      if (b.IsValid())
        bounds.ExpandToInclude(b);
    }
  }

  if (!bounds.IsValid())
    bounds = ezBoundingBoxSphere(ezVec3::ZeroVector(), ezVec3(1, 1, 1), 2);

  return bounds;
}

ezEngineProcessDocumentContext::ezEngineProcessDocumentContext()
{
  m_pWorld = nullptr;
  m_uiThumbnailConvergenceFrames = 0;
  m_uiThumbnailWidth = 0;
  m_uiThumbnailHeight = 0;
  m_pThumbnailViewContext = nullptr;
}

ezEngineProcessDocumentContext::~ezEngineProcessDocumentContext()
{
  EZ_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'ezEngineProcessDocumentContext::DestroyDocumentContext'");
}

void ezEngineProcessDocumentContext::Initialize(const ezUuid& DocumentGuid, ezEngineProcessCommunicationChannel* pIPC)
{
  m_DocumentGuid = DocumentGuid;
  m_pIPC = pIPC;

  ezStringBuilder tmp;
  ezWorldDesc desc(ezConversionUtils::ToString(m_DocumentGuid, tmp));
  m_pWorld = ezGameApplication::GetGameApplicationInstance()->CreateWorld(desc);

  m_Context.m_pWorld = m_pWorld;
  m_Mirror.InitReceiver(&m_Context);

  OnInitialize();
}

void ezEngineProcessDocumentContext::Deinitialize()
{
  ClearViewContexts();
  m_Mirror.Clear();
  m_Mirror.DeInit();
  m_Context.Clear();

  OnDeinitialize();

  CleanUpContextSyncObjects();

  ezGameApplication::GetGameApplicationInstance()->DestroyWorld(m_pWorld);
  m_pWorld = nullptr;
}

void ezEngineProcessDocumentContext::SendProcessMessage(ezProcessMessage* pMsg)
{
  m_pIPC->SendMessage(pMsg);
}

void ezEngineProcessDocumentContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  EZ_LOCK(m_pWorld->GetWriteMarker());

  const bool bIsRemoteProcess = ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode();

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEntityMsgToEngine>())
  {
    const ezEntityMsgToEngine* pMsg2 = static_cast<const ezEntityMsgToEngine*>(pMsg);
    m_Mirror.ApplyOp(const_cast<ezObjectChange&>(pMsg2->m_change));
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineSyncObjectMsg>())
  {
    const ezEditorEngineSyncObjectMsg* pMsg2 = static_cast<const ezEditorEngineSyncObjectMsg*>(pMsg);

    ProcessEditorEngineSyncObjectMsg(*pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectTagMsgToEngine>())
  {
    const ezObjectTagMsgToEngine* pMsg2 = static_cast<const ezObjectTagMsgToEngine*>(pMsg);

    SetTagOnObject(pMsg2->m_ObjectGuid, pMsg2->m_sTag, pMsg2->m_bSetTag, pMsg2->m_bApplyOnAllChildren);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezExportDocumentMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const ezExportDocumentMsgToEngine* pMsg2 = static_cast<const ezExportDocumentMsgToEngine*>(pMsg);
    ezExportDocumentMsgToEditor ret;
    ret.m_DocumentGuid = pMsg->m_DocumentGuid;
    ret.m_bOutputSuccess = ExportDocument(pMsg2);
    if (!ret.m_bOutputSuccess)
    {
      ezLog::Error("Could not export to file '{0}'.", pMsg2->m_sOutputFile);
    }

    SendProcessMessage(&ret);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCreateThumbnailMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    ezResourceManager::ReloadAllResources(false);
    const ezCreateThumbnailMsgToEngine* pMsg2 = static_cast<const ezCreateThumbnailMsgToEngine*>(pMsg);
    // As long as the thumbnail context is alive, we will trigger UpdateThumbnailViewContext
    // inside the UpdateDocumentContext function until the thumbnail rendering has converged and
    // the data is send back as a response.
    CreateThumbnailViewContext(pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineViewMsg>())
  {
    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
    {
      UpdateSyncObjects();
    }

    const ezEditorEngineViewMsg* pViewMsg = static_cast<const ezEditorEngineViewMsg*>(pMsg);
    EZ_ASSERT_DEV(pViewMsg->m_uiViewID < 0xFFFFFFFF, "Invalid view ID in '{0}'", pMsg->GetDynamicRTTI()->GetTypeName());

    if (pViewMsg->m_uiViewID >= m_ViewContexts.GetCount())
    {
      m_ViewContexts.SetCount(pViewMsg->m_uiViewID + 1);
    }

    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewDestroyedMsgToEngine>())
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] != nullptr)
      {
        DestroyViewContext(m_ViewContexts[pViewMsg->m_uiViewID]);
        m_ViewContexts[pViewMsg->m_uiViewID] = nullptr;

        ezLog::Debug("Destroyed View {0}", pViewMsg->m_uiViewID);
      }
    }
    else
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] == nullptr)
      {
        if (!ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = CreateViewContext();
        }
        else
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = EZ_DEFAULT_NEW(ezRemoteEngineProcessViewContext, this);
        }

        m_ViewContexts[pViewMsg->m_uiViewID]->SetViewID(pViewMsg->m_uiViewID);
      }

      m_ViewContexts[pViewMsg->m_uiViewID]->HandleViewMessage(pViewMsg);
    }

    return;
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewHighlightMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const ezViewHighlightMsgToEngine* pMsg2 = static_cast<const ezViewHighlightMsgToEngine*>(pMsg);

    m_Context.m_uiHighlightID = m_Context.m_ComponentPickingMap.GetHandle(pMsg2->m_HighlightObject);

    if (m_Context.m_uiHighlightID == 0)
      m_Context.m_uiHighlightID = m_Context.m_OtherPickingMap.GetHandle(pMsg2->m_HighlightObject);
  }
}

void ezEngineProcessDocumentContext::AddSyncObject(ezEditorEngineSyncObject* pSync)
{
  pSync->Configure(m_DocumentGuid, [this](ezEditorEngineSyncObject* pSync)
  {
    RemoveSyncObject(pSync);
  });

  m_SyncObjects[pSync->GetGuid()] = pSync;
}

void ezEngineProcessDocumentContext::RemoveSyncObject(ezEditorEngineSyncObject* pSync)
{
  m_SyncObjects.Remove(pSync->GetGuid());
}

ezEditorEngineSyncObject* ezEngineProcessDocumentContext::FindSyncObject(const ezUuid& guid)
{
  return m_SyncObjects.GetValueOrDefault(guid, nullptr);
}

void ezEngineProcessDocumentContext::ResourceEventHandler(const ezResourceEvent& e)
{
  // If any resource has changed we assume it's not a good idea to take a screenshot now,
  // so we restart the counter.
  // TODO: Even in combination with ezResourceManager::FinishLoadingOfResources we end up with
  // broken thumbnails :-/

  //ezLog::Debug("Resource changed, resetting counter");
  //m_uiThumbnailConvergenceFrames = 0;
}

void ezEngineProcessDocumentContext::ClearViewContexts()
{
  for (auto* pContext : m_ViewContexts)
  {
    DestroyViewContext(pContext);
  }

  m_ViewContexts.Clear();
}


void ezEngineProcessDocumentContext::CleanUpContextSyncObjects()
{
  while (!m_SyncObjects.IsEmpty())
  {
    auto it = m_SyncObjects.GetIterator();
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }
}

void ezEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const ezEditorEngineSyncObjectMsg& msg)
{
  auto it = m_SyncObjects.Find(msg.m_ObjectGuid);

  if (msg.m_sObjectType.IsEmpty())
  {
    // object has been deleted!
    if (it.IsValid())
    {
      it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
    }

    return;
  }

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(msg.m_sObjectType);
  ezEditorEngineSyncObject* pSyncObject = nullptr;
  bool bSetOwner = false;

  if (pRtti == nullptr)
  {
    ezLog::Error("Cannot sync object of type unknown '{0}' to engine process", msg.m_sObjectType);
    return;
  }

  if (!it.IsValid())
  {
    // object does not yet exist
    EZ_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '{0}' does not have a default allocator", msg.m_sObjectType);
    void* pObject = pRtti->GetAllocator()->Allocate();

    pSyncObject = static_cast<ezEditorEngineSyncObject*>(pObject);
    bSetOwner = true;
  }
  else
  {
    pSyncObject = it.Value();
  }

  ezRawMemoryStreamReader reader(msg.m_ObjectData);

  ezReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, pSyncObject);

  if (bSetOwner)
  {
    AddSyncObject(pSyncObject);
  }

  pSyncObject->SetModified(true);
}


void ezEngineProcessDocumentContext::Reset()
{
  ezUuid guid = m_DocumentGuid;
  auto ipc = m_pIPC;

  Deinitialize();

  Initialize(guid, ipc);
}


void ezEngineProcessDocumentContext::ClearExistingObjects()
{
  m_Context.DeleteExistingObjects();
}

bool ezEngineProcessDocumentContext::PendingOperationInProgress() const
{
  return m_pThumbnailViewContext != nullptr;
}

void ezEngineProcessDocumentContext::UpdateDocumentContext()
{
  if (m_pThumbnailViewContext)
  {
    m_uiThumbnailConvergenceFrames++;

    //ezLog::Debug("Updating document context for thumbnail: {0}", m_uiThumbnailConvergenceFrames);

    // Once all resources are loaded and UpdateThumbnailViewContext returns true,
    // we render 'ThumbnailConvergenceFramesTarget' frames and than download it.
    if (ezResourceManager::FinishLoadingOfResources())
    {
      //ezLog::Debug("Resources loaded, Resetting convergence counter");
      m_uiThumbnailConvergenceFrames = 0;
    }

    if (!UpdateThumbnailViewContext(m_pThumbnailViewContext))
    {
      //ezLog::Debug("Not updated thumbnail context, Resetting convergence counter");
      m_uiThumbnailConvergenceFrames = 0;
    }

    if (m_uiThumbnailConvergenceFrames > ThumbnailConvergenceFramesTarget)
    {
      //ezLog::Debug("Convergence > threshold, storing thumbnail");

      ezCreateThumbnailMsgToEditor ret;
      ret.m_DocumentGuid = GetDocumentGuid();

      // Download image
      {
        //ezLog::Success("Reading back Thumbnail");

        ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hThumbnailColorRT);

        ezGALSystemMemoryDescription MemDesc;
        {
          MemDesc.m_uiRowPitch = 4 * m_uiThumbnailWidth;
          MemDesc.m_uiSlicePitch = 4 * m_uiThumbnailWidth * m_uiThumbnailHeight;
        }

        ezImage image;
        image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
        image.SetWidth(m_uiThumbnailWidth);
        image.SetHeight(m_uiThumbnailHeight);
        image.AllocateImageData();
        EZ_ASSERT_DEV(m_uiThumbnailWidth * m_uiThumbnailHeight * 4 == image.GetDataSize(), "Thumbnail ezImage has different size than data buffer!");

        MemDesc.m_pData = image.GetDataPointer<ezUInt8>();
        ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
        ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hThumbnailColorRT, &SysMemDescs);

        ezImage imageSwap;
        ezImage* pImage = &image;
        ezImage* pImageSwap = &imageSwap;
        for (ezUInt32 uiSuperscaleFactor = ThumbnailSuperscaleFactor; uiSuperscaleFactor > 1; uiSuperscaleFactor /= 2)
        {
          ezImageUtils::ScaleDownHalf(*pImage, *pImageSwap);
          ezMath::Swap(pImage, pImageSwap);
        }


        ret.m_ThumbnailData.SetCountUninitialized((m_uiThumbnailWidth / ThumbnailSuperscaleFactor)
        * (m_uiThumbnailHeight / ThumbnailSuperscaleFactor) * 4);
        ezMemoryUtils::Copy(ret.m_ThumbnailData.GetData(), pImage->GetDataPointer<ezUInt8>(), ret.m_ThumbnailData.GetCount());
      }

      DestroyThumbnailViewContext();

      // Send response.
      SendProcessMessage(&ret);
    }
    else
    {
      //ezLog::Info("Rendering Thumbnail");
      m_pThumbnailViewContext->Redraw(false);
    }
  }
}

bool ezEngineProcessDocumentContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezLog::Error("Export document not implemented for '{0}'", GetDynamicRTTI()->GetTypeName());
  return false;
}


void ezEngineProcessDocumentContext::CreateThumbnailViewContext(const ezCreateThumbnailMsgToEngine* pMsg)
{
  EZ_ASSERT_DEV(!ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode(), "Wrong mode for thumbnail creation");
  EZ_ASSERT_DEV(m_pThumbnailViewContext == nullptr, "Thumbnail rendering already in progress.");
  EZ_CHECK_AT_COMPILETIME_MSG((ThumbnailSuperscaleFactor & (ThumbnailSuperscaleFactor - 1)) == 0, "ThumbnailSuperscaleFactor must be power of 2.");
  m_uiThumbnailConvergenceFrames = 0;
  m_uiThumbnailWidth = pMsg->m_uiWidth * ThumbnailSuperscaleFactor;
  m_uiThumbnailHeight = pMsg->m_uiHeight * ThumbnailSuperscaleFactor;
  m_pThumbnailViewContext = CreateViewContext();

  // make sure the world is not simulating while making a screenshot
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());
    m_bWorldSimStateBeforeThumbnail = m_pWorld->GetWorldSimulationEnabled();
    m_pWorld->SetWorldSimulationEnabled(false);
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Create render target for picking
  ezGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView = false;
  tcd.m_bAllowUAV = false;
  tcd.m_bCreateRenderTarget = true;
  tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type = ezGALTextureType::Texture2D;
  tcd.m_uiWidth = m_uiThumbnailWidth;
  tcd.m_uiHeight = m_uiThumbnailHeight;

  m_hThumbnailColorRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = ezGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = false;

  m_hThumbnailDepthRT = pDevice->CreateTexture(tcd);

  m_ThumbnailRenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hThumbnailColorRT))
    .SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hThumbnailDepthRT));
  m_pThumbnailViewContext->SetupRenderTarget(m_ThumbnailRenderTargetSetup, m_uiThumbnailWidth, m_uiThumbnailHeight);

  UpdateThumbnailViewContext(m_pThumbnailViewContext);
  m_pThumbnailViewContext->Redraw(false);

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezEngineProcessDocumentContext::ResourceEventHandler, this));

  OnThumbnailViewContextCreated();
}

void ezEngineProcessDocumentContext::DestroyThumbnailViewContext()
{
  OnDestroyThumbnailViewContext();

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessDocumentContext::ResourceEventHandler, this));

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  DestroyViewContext(m_pThumbnailViewContext);
  m_pThumbnailViewContext = nullptr;

  m_ThumbnailRenderTargetSetup.DestroyAllAttachedViews();
  if (!m_hThumbnailColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hThumbnailColorRT);
    m_hThumbnailColorRT.Invalidate();
  }

  if (!m_hThumbnailDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hThumbnailDepthRT);
    m_hThumbnailDepthRT.Invalidate();
  }

  m_pWorld->SetWorldSimulationEnabled(m_bWorldSimStateBeforeThumbnail);
}

bool ezEngineProcessDocumentContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  ezLog::Error("UpdateThumbnailViewContext not implemented for '{0}'", GetDynamicRTTI()->GetTypeName());
  return true;
}

void ezEngineProcessDocumentContext::SetTagOnObject(const ezUuid& object, const char* szTag, bool bSet, bool recursive)
{
  ezGameObjectHandle hObject = m_Context.m_GameObjectMap.GetHandle(object);

  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTag);

  ezGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (recursive)
    {
      if (bSet)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (bSet)
        pObject->GetTags().Set(tag);
      else
        pObject->GetTags().Remove(tag);
    }
  }
}

void ezEngineProcessDocumentContext::SetTagRecursive(ezGameObject* pObject, const ezTag& tag)
{
  pObject->GetTags().Set(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {

    SetTagRecursive(&itChild.Current(), tag);
  }
}

void ezEngineProcessDocumentContext::ClearTagRecursive(ezGameObject* pObject, const ezTag& tag)
{
  pObject->GetTags().Remove(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    ClearTagRecursive(&itChild.Current(), tag);
  }
}

void ezEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto* pSyncObject : m_SyncObjects)
  {
    if (pSyncObject->GetModified())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      EZ_LOCK(m_pWorld->GetWriteMarker());

      if (pSyncObject->SetupForEngine(m_pWorld, m_Context.m_uiNextComponentPickingID))
      {
        m_Context.m_OtherPickingMap.RegisterObject(pSyncObject->GetGuid(), m_Context.m_uiNextComponentPickingID);
        ++m_Context.m_uiNextComponentPickingID;
      }

      pSyncObject->UpdateForEngine(m_pWorld);
    }
  }
}

