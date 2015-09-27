#include <PCH.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>

#include <RendererCore/Meshes/MeshComponent.h>
#include <GameUtils/Components/RotorComponent.h>
#include <GameUtils/Components/SliderComponent.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneContext, ezEngineProcessDocumentContext, 1, ezRTTIDefaultAllocator<ezSceneContext>);
EZ_BEGIN_PROPERTIES
EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "ezScene")
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();


void ezSceneContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
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
      EZ_LOCK(m_pWorld->GetReadMarker());

      for (const auto& obj : m_Selection)
      {
        ezGameObject* pObj;
        if (!m_pWorld->TryGetObject(obj, pObj))
          continue;

        auto b = pObj->GetGlobalBounds();

        if (bounds.IsValid())
          bounds.ExpandToInclude(b);
        else
          bounds = b;
      }
    }

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
  EZ_LOCK(m_pWorld->GetWriteMarker());

  /// \todo Plugin concept to allow custom initialization
  m_pWorld->CreateComponentManager<ezMeshComponentManager>();
  m_pWorld->CreateComponentManager<ezRotorComponentManager>();
  m_pWorld->CreateComponentManager<ezSliderComponentManager>();
}

ezEngineProcessViewContext* ezSceneContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezViewContext, this);
}

void ezSceneContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

void ezSceneContext::HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg)
{
  m_Selection.Clear();

  ezStringBuilder sSel = pMsg->m_sSelection;
  ezStringBuilder sGuid;

  while (!sSel.IsEmpty())
  {
    sGuid.SetSubString_ElementCount(sSel.GetData() + 1, 40);
    sSel.Shrink(41, 0);

    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

    auto hObject = m_GameObjectMap.GetHandle(guid);

    if (!hObject.IsInvalidated())
      m_Selection.PushBack(hObject);
  }
}


