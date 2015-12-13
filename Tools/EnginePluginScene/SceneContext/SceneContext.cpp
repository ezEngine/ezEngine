#include <PCH.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <GameUtils/Components/SliderComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <Core/Scene/Scene.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, 1, ezRTTIDefaultAllocator<ezSceneContext>);
EZ_BEGIN_PROPERTIES
EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "ezScene;ezPrefab"),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezSceneContext::ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds)
{
  /// \todo Work around for objects without bounds (?)
  auto b = pObj->GetGlobalBounds();

  if (b.IsValid())
    bounds.ExpandToInclude(b);

  auto it = pObj->GetChildren();

  while (it.IsValid())
  {
    ComputeHierarchyBounds(it, bounds);
    it.Next();
  }
}

void ezSceneContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezSceneSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

     auto msg = static_cast<const ezSceneSettingsMsgToEngine*>(pMsg);

    const bool bSimulate = msg->m_bSimulateWorld;
    m_bRenderSelectionOverlay = msg->m_bRenderOverlay;

    if (bSimulate != GetScene()->GetWorld()->GetWorldSimulationEnabled())
    {
      ezLog::Info("World Simulation %s", bSimulate ? "enabled" : "disabled");
      GetScene()->GetWorld()->SetWorldSimulationEnabled(bSimulate);

      if (bSimulate)
      {
        GetScene()->ReinitSceneModules();
      }
    }

    GetScene()->GetWorld()->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    GetScene()->Update();

    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezObjectSelectionMsgToEngine>())
  {
    HandleSelectionMsg(static_cast<const ezObjectSelectionMsgToEngine*>(pMsg));
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxMsgToEngine>())
  {
    if (m_Selection.IsEmpty())
      return;

    ezBoundingBoxSphere bounds;
    bounds.SetInvalid();

    {
      auto pWorld = GetScene()->GetWorld();

      EZ_LOCK(pWorld->GetReadMarker());

      for (const auto& obj : m_Selection)
      {
        ezGameObject* pObj;
        if (!pWorld->TryGetObject(obj, pObj))
          continue;

        ComputeHierarchyBounds(pObj, bounds);
      }

      // if there are no valid bounds, at all, use dummy bounds for each object
      if (!bounds.IsValid())
      {
        for (const auto& obj : m_Selection)
        {
          ezGameObject* pObj;
          if (!pWorld->TryGetObject(obj, pObj))
            continue;

          bounds.ExpandToInclude(ezBoundingBoxSphere(pObj->GetGlobalPosition(), ezVec3(0.5f), 0.5f));
        }
      }
    }

    EZ_ASSERT_DEV(bounds.IsValid() && !bounds.IsNaN(), "Invalid bounds");

    const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

    ezQuerySelectionBBoxResultMsgToEditor res;
    res.m_uiViewID = msg->m_uiViewID;
    res.m_iPurpose = msg->m_iPurpose;
    res.m_vCenter = bounds.m_vCenter;
    res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
    res.m_DocumentGuid = pMsg->m_DocumentGuid;

    SendProcessMessage(&res);

    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezSceneContext::OnInitialize()
{
  auto pWorld = GetScene()->GetWorld();
  EZ_LOCK(pWorld->GetWriteMarker());

  /// \todo Plugin concept to allow custom initialization
  pWorld->CreateComponentManager<ezMeshComponentManager>();
  pWorld->CreateComponentManager<ezRotorComponentManager>();
  pWorld->CreateComponentManager<ezSliderComponentManager>();
}

ezEngineProcessViewContext* ezSceneContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezSceneViewContext, this);
}

void ezSceneContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezSceneContext::HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();
  m_SelectionWithChildrenSet.Clear();
  m_SelectionWithChildren.Clear();

  ezStringBuilder sSel = pMsg->m_sSelection;
  ezStringBuilder sGuid;

  auto pWorld = GetScene()->GetWorld();
  EZ_LOCK(pWorld->GetReadMarker());

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = m_Context.m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
    {
      m_Selection.PushBack(hObject);

      ezGameObject* pObject;
      if (pWorld->TryGetObject(hObject, pObject))
        InsertSelectedChildren(pObject);
    }
  }

  for (auto it = m_SelectionWithChildrenSet.GetIterator(); it.IsValid(); ++it)
  {
    m_SelectionWithChildren.PushBack(it.Key());
  }
}

void ezSceneContext::InsertSelectedChildren(const ezGameObject* pObject)
{
  m_SelectionWithChildrenSet.Insert(pObject->GetHandle());

  auto it = pObject->GetChildren();

  while (it.IsValid())
  {
    InsertSelectedChildren(it);

    it.Next();
  }
}
