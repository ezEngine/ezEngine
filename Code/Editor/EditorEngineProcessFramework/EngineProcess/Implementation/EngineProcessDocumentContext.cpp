#include <EditorEngineProcessFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Prefabs/PrefabReferenceComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEngineProcessDocumentContext, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezHashTable<ezUuid, ezEngineProcessDocumentContext*> ezEngineProcessDocumentContext::s_DocumentContexts;

ezEngineProcessDocumentContext* ezEngineProcessDocumentContext::GetDocumentContext(ezUuid guid)
{
  ezEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void ezEngineProcessDocumentContext::AddDocumentContext(
  ezUuid guid, ezEngineProcessDocumentContext* pContext, ezEngineProcessCommunicationChannel* pIPC)
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
  m_Context.m_Events.AddEventHandler(ezMakeDelegate(&ezEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

ezEngineProcessDocumentContext::~ezEngineProcessDocumentContext()
{
  EZ_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'ezEngineProcessDocumentContext::DestroyDocumentContext'");

  m_Context.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

void ezEngineProcessDocumentContext::Initialize(const ezUuid& DocumentGuid, ezEngineProcessCommunicationChannel* pIPC)
{
  m_DocumentGuid = DocumentGuid;
  m_pIPC = pIPC;

  ezStringBuilder tmp;
  ezWorldDesc desc(ezConversionUtils::ToString(m_DocumentGuid, tmp));
  desc.m_bReportErrorWhenStaticObjectMoves = false;

  m_pWorld = EZ_DEFAULT_NEW(ezWorld, desc);
  m_pWorld->SetGameObjectReferenceResolver(ezMakeDelegate(&ezEngineProcessDocumentContext::ResolveStringToGameObjectHandle, this));

  m_Context.m_pWorld = m_pWorld.Borrow();
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

    ezRttiConverterObject target = m_Context.GetObjectByGUID(pMsg2->m_change.m_Root);

    if (target.m_pType == nullptr || target.m_pObject == nullptr)
      return;

    if (target.m_pType == ezGetStaticRTTI<ezGameObject>())
    {
      ezGameObject* pObject = static_cast<ezGameObject*>(target.m_pObject);
      if (pObject != nullptr && pObject->IsStatic())
      {
        ezRenderWorld::DeleteCachedRenderDataForObjectRecursive(pObject);
      }
    }
    else if (target.m_pType->IsDerivedFrom<ezComponent>())
    {
      ezComponent* pComponent = static_cast<ezComponent*>(target.m_pObject);
      if (pComponent != nullptr && pComponent->GetOwner()->IsStatic())
      {
        ezRenderWorld::DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
      }
    }
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

    m_ViewContexts.EnsureCount(pViewMsg->m_uiViewID + 1);

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
  pSync->Configure(m_DocumentGuid, [this](ezEditorEngineSyncObject* pSync) { RemoveSyncObject(pSync); });

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
    void* pObject = pRtti->GetAllocator()->Allocate<void>();

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

void ezEngineProcessDocumentContext::OnInitialize() {}
void ezEngineProcessDocumentContext::OnDeinitialize() {}

bool ezEngineProcessDocumentContext::PendingOperationInProgress() const
{
  auto pState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetWorld());
  return m_pThumbnailViewContext != nullptr || pState != nullptr;
}

void ezEngineProcessDocumentContext::UpdateDocumentContext()
{
  if (ezEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    // in remote mode simply redraw all all views every time a context is updated
    for (auto pView : m_ViewContexts)
    {
      if (pView)
        pView->Redraw(false);
    }
  }

  {
    // If we have a running game state we always want to render it (e.g. play the game).
    auto pState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetWorld());
    if (pState != nullptr)
    {
      pState->ScheduleRendering();
    }
  }

  if (m_pThumbnailViewContext)
  {
    ezResourceManager::ForceNoFallbackAcquisition(3);
    m_uiThumbnailConvergenceFrames++;

    if (!UpdateThumbnailViewContext(m_pThumbnailViewContext))
    {
      m_uiThumbnailConvergenceFrames = 0;
    }

    if (m_uiThumbnailConvergenceFrames > ThumbnailConvergenceFramesTarget)
    {
      ezCreateThumbnailMsgToEditor ret;
      ret.m_DocumentGuid = GetDocumentGuid();

      // Download image
      {
        ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(m_hThumbnailColorRT);

        ezGALSystemMemoryDescription MemDesc;
        {
          MemDesc.m_uiRowPitch = 4 * m_uiThumbnailWidth;
          MemDesc.m_uiSlicePitch = 4 * m_uiThumbnailWidth * m_uiThumbnailHeight;
        }

        ezImageHeader header;
        header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
        header.SetWidth(m_uiThumbnailWidth);
        header.SetHeight(m_uiThumbnailHeight);
        ezImage image;
        image.ResetAndAlloc(header);
        EZ_ASSERT_DEV(static_cast<ezUInt64>(m_uiThumbnailWidth) * static_cast<ezUInt64>(m_uiThumbnailHeight) * 4 == header.ComputeDataSize(),
          "Thumbnail ezImage has different size than data buffer!");

        MemDesc.m_pData = image.GetPixelPointer<ezUInt8>();
        ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
        ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(m_hThumbnailColorRT, &SysMemDescs);

        ezImage imageSwap;
        ezImage* pImage = &image;
        ezImage* pImageSwap = &imageSwap;
        for (ezUInt32 uiSuperscaleFactor = ThumbnailSuperscaleFactor; uiSuperscaleFactor > 1; uiSuperscaleFactor /= 2)
        {
          ezImageUtils::Scale(*pImage, *pImageSwap, pImage->GetWidth() / 2, pImage->GetHeight() / 2);
          ezMath::Swap(pImage, pImageSwap);
        }


        ret.m_ThumbnailData.SetCountUninitialized(
          (m_uiThumbnailWidth / ThumbnailSuperscaleFactor) * (m_uiThumbnailHeight / ThumbnailSuperscaleFactor) * 4);
        ezMemoryUtils::Copy(ret.m_ThumbnailData.GetData(), pImage->GetPixelPointer<ezUInt8>(), ret.m_ThumbnailData.GetCount());
      }

      DestroyThumbnailViewContext();

      // Send response.
      SendProcessMessage(&ret);
    }
    else
    {
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

  ezResourceManager::ForceNoFallbackAcquisition(3);
  OnThumbnailViewContextRequested();
  UpdateThumbnailViewContext(m_pThumbnailViewContext);

  // disable editor specific render passes in the thumbnail view
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_pThumbnailViewContext->GetViewHandle(), pView))
  {
    pView->SetViewRenderMode(ezViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "Active", false);
  }

  m_pThumbnailViewContext->Redraw(false);

  OnThumbnailViewContextCreated();
}

void ezEngineProcessDocumentContext::DestroyThumbnailViewContext()
{
  OnDestroyThumbnailViewContext();

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

void ezEngineProcessDocumentContext::OnThumbnailViewContextCreated() {}
void ezEngineProcessDocumentContext::OnDestroyThumbnailViewContext() {}

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

    SetTagRecursive(itChild, tag);
  }
}

void ezEngineProcessDocumentContext::ClearTagRecursive(ezGameObject* pObject, const ezTag& tag)
{
  pObject->GetTags().Remove(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    ClearTagRecursive(itChild, tag);
  }
}

void ezEngineProcessDocumentContext::WorldRttiConverterContextEventHandler(const ezWorldRttiConverterContext::Event& e)
{
  if (e.m_Type == ezWorldRttiConverterContext::Event::Type::GameObjectCreated)
  {
    // see whether the newly created object is already referenced by other objects
    auto it = m_GoRef_ReferencedBy.Find(e.m_ObjectGuid);
    if (!it.IsValid())
      return;

    ezStringBuilder tmp;

    // iterate over all objects that may reference the new object
    for (const auto& ref : it.Value())
    {
      const ezUuid compGuid = ref.m_ReferencedByComponent;
      if (!compGuid.IsValid())
        continue;

      // check whether the object that references the new object is already known (may be dead or not yet created as well)
      ezComponentHandle hRefComp = m_Context.m_ComponentMap.GetHandle(compGuid);

      if (hRefComp.IsInvalidated())
        continue;

      ezComponent* pRefComp = nullptr;
      if (!GetWorld()->TryGetComponent(hRefComp, pRefComp))
        continue;

      if (!ezStringUtils::IsNullOrEmpty(ref.m_szComponentProperty))
      {
        // in this case, a 'regular' component+property reference the new object
        // so we can just re-apply the reference (by setting the property again)
        // and thus trigger that the other object updates/fixes its internal state

        ezAbstractProperty* pAbsProp = pRefComp->GetDynamicRTTI()->FindPropertyByName(ref.m_szComponentProperty);
        if (pAbsProp == nullptr)
          continue;

        if (pAbsProp->GetCategory() != ezPropertyCategory::Member)
          continue;

        ezConversionUtils::ToString(e.m_ObjectGuid, tmp);

        ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pAbsProp), pRefComp, tmp.GetData());
      }
      else
      {
        // in this case, the object was referenced from a component inside a prefab
        // the problem with prefabs is, that their internal objects and components are recreated all the time
        // (e.g. every time any exposed parameter is changed)
        // and since we have no GUIDs for the internal objects, we cannot directly update those internal references
        // we can, however, just update the entire prefab, which will kill all internal objects and recreate them

        ezPrefabReferenceComponent* pPrefab = ezDynamicCast<ezPrefabReferenceComponent*>(pRefComp);
        EZ_ASSERT_DEV(pPrefab != nullptr, "Game-Object reference update: Expected an ezPrefabReferenceComponent");

        ezPrefabReferenceComponentManager* pManager = ezStaticCast<ezPrefabReferenceComponentManager*>(pPrefab->GetOwningManager());
        pManager->AddToUpdateList(pPrefab);
      }
    }
  }
  else if (e.m_Type == ezWorldRttiConverterContext::Event::Type::GameObjectDeleted)
  {
    m_GoRef_ReferencesTo.Remove(e.m_ObjectGuid);
  }
}

/// Tries to resolve a 'reference' (given in pData) to an ezGameObject.
/// hThis is the 'owner' of the reference and szComponentProperty is the name of the reference property in that component.
///
/// There are two different use cases:
///
///  1) hThis is invalid and szComponentProperty is null:
///
///     This is used by ezPrefabReferenceComponent::SerializeComponent() to check whether a string represents a game object reference.
///     It may be any arbitrary string and thus must not assert.
///     In this case a reference is always a stringyfied GUID.
///     Since this is only used for scene export, only the lookup shall be done and nothing else.
///
///  2) hThis and szComponentProperty represent a valid component+property combination:
///
///     This is called at edit time whenever a reference property is queried, which also happens whenever a reference is modified.
///     In this case we need to maintain two maps:
///       one that know which object references which other objects
///       one that knows by which other objects an object is referenced
///     These are needed to fix up references during undo/redo when objects get deleted and recreated.
///     Ie. when an object that has references or is referenced gets deleted and then undo restores it, the references should appear as well.
///
ezGameObjectHandle ezEngineProcessDocumentContext::ResolveStringToGameObjectHandle(
  const void* pData, ezComponentHandle hThis, const char* szComponentProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pData);

  if (hThis.IsInvalidated() && szComponentProperty == nullptr)
  {
    // This code path is used by ezPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!ezConversionUtils::IsStringUuid(szTargetGuid))
      return ezGameObjectHandle();

    // convert string to GUID and check if references a known object
    return m_Context.m_GameObjectMap.GetHandle(ezConversionUtils::ConvertStringToUuid(szTargetGuid));
  }



  ezUuid srcComponentGuid = m_Context.m_ComponentMap.GetGuid(hThis);
  if (!srcComponentGuid.IsValid())
  {
    // if we do not know hThis, it is usually a component that was created by a prefab instance
    // since we need hThis/srcComponentGuid to update our tables who references whom, we now try to walk up the node hierarchy
    // until we find a known game object
    // there, currently, we assume to find an ezPrefabReferenceComponent, which will be used as srcComponentGuid

    ezComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hThis, pComponent))
      return ezGameObjectHandle();

    ezGameObject* pOwner = pComponent->GetOwner();

    // search the parents for a known game object
    while (pOwner)
    {
      srcComponentGuid = m_Context.m_GameObjectMap.GetGuid(pOwner->GetHandle());

      if (srcComponentGuid.IsValid())
        break;

      pOwner = pOwner->GetParent();
    }

    // currently we assume all these conditions should be met
    // have to check with reality, though
    EZ_ASSERT_DEV(pOwner != nullptr, "Expected a known top level object");
    EZ_ASSERT_DEV(srcComponentGuid.IsValid(), "Expected a known top level object");

    // for the time being we assume to find a prefab component here, but this may change if new use cases come up
    ezPrefabReferenceComponent* pPrefabComponent;
    if (pOwner->TryGetComponentOfBaseType(pPrefabComponent))
    {
      srcComponentGuid = m_Context.m_ComponentMap.GetGuid(pPrefabComponent->GetHandle());
      EZ_ASSERT_DEV(srcComponentGuid.IsValid(), "");

      // tag this reference as being special
      szComponentProperty = nullptr;
    }
    else
    {
      // this could probably happen if we have a component that creates sub-objects even at edit time
      // and is not a prefab reference component
      // and uses game object references
      EZ_ASSERT_DEV(false, "Expected a known ezPrefabReferenceComponent as top-level object");
      return ezGameObjectHandle();
    }
  }


  ezUuid newTargetGuid;
  ezUuid oldTargetGuid;

  if (ezConversionUtils::IsStringUuid(szTargetGuid))
  {
    newTargetGuid = ezConversionUtils::ConvertStringToUuid(szTargetGuid);
  }
  else
  {
    EZ_ASSERT_DEV(ezStringUtils::IsNullOrEmpty(szTargetGuid), "Expected GUID references");
  }


  // overview for the steps below:
  //
  // check if m_GoRef_ReferencesTo[srcComponentGuid] already maps from [szComponentProperty] to something -> update (remove if pData is empty/invalid)
  // otherwise add reference
  //
  // if already mapped to something, remove reference from m_GoRef_ReferencedBy
  // then add new reference to m_GoRef_ReferencedBy



  // update which object this component+property map to
  {
    auto& referencesTo = m_GoRef_ReferencesTo[srcComponentGuid];

    // check all references from the src component
    for (ezUInt32 i = 0; i < referencesTo.GetCount(); ++i)
    {
      // if this is the desired property, update it
      if (ezStringUtils::IsEqual(referencesTo[i].m_szComponentProperty, szComponentProperty))
      {
        // retrieve previous reference, needed to update m_GoRef_ReferencedBy
        oldTargetGuid = referencesTo[i].m_ReferenceToGameObject;

        // write the new reference
        if (newTargetGuid.IsValid())
        {
          referencesTo[i].m_ReferenceToGameObject = newTargetGuid;
        }
        else
        {
          referencesTo.RemoveAtAndSwap(i);
        }

        goto ref_to_is_updated;
      }
    }

    // if we end up here, the reference-to was previously unknown and must be added
    if (newTargetGuid.IsValid())
    {
      auto& refTo = referencesTo.ExpandAndGetRef();
      refTo.m_szComponentProperty = szComponentProperty;
      refTo.m_ReferenceToGameObject = newTargetGuid;
    }
  }

ref_to_is_updated:

  // only need to updated m_GoRef_ReferencedBy if the reference has actually changed
  if (oldTargetGuid != newTargetGuid)
  {
    // if we referenced another object previously, remove the 'referenced-by' link to the old object
    if (oldTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[oldTargetGuid];

      for (ezUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid &&
            ezStringUtils::IsEqual(referencedBy[i].m_szComponentProperty, szComponentProperty))
        {
          referencedBy.RemoveAtAndSwap(i);
          break;
        }
      }
    }

    // if we are now referencing a valid object, add a 'referenced-by' link to the new object
    if (newTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[newTargetGuid];

      // this loop is currently only to validate that no bugs creeped in
      for (ezUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid &&
            ezStringUtils::IsEqual(referencedBy[i].m_szComponentProperty, szComponentProperty))
        {
          EZ_REPORT_FAILURE("Go-reference was not updated correctly");
        }
      }

      // add the back-reference
      auto& newRef = referencedBy.ExpandAndGetRef();
      newRef.m_ReferencedByComponent = srcComponentGuid;
      newRef.m_szComponentProperty = szComponentProperty;
    }
  }

  // just an optimization for the common case
  if (!newTargetGuid.IsValid())
    return ezGameObjectHandle();

  return m_Context.m_GameObjectMap.GetHandle(newTargetGuid);
}

void ezEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto it : m_SyncObjects)
  {
    auto pSyncObject = it.Value();

    if (pSyncObject->GetModified())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      EZ_LOCK(m_pWorld->GetWriteMarker());

      if (pSyncObject->SetupForEngine(m_pWorld.Borrow(), m_Context.m_uiNextComponentPickingID))
      {
        m_Context.m_OtherPickingMap.RegisterObject(pSyncObject->GetGuid(), m_Context.m_uiNextComponentPickingID);
        ++m_Context.m_uiNextComponentPickingID;
      }

      pSyncObject->UpdateForEngine(m_pWorld.Borrow());
    }
  }
}
