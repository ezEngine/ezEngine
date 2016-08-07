#include <PCH.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/IPC/SyncObject.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Logging/Log.h>
#include <QKeyEvent>
#include <RendererCore/Meshes/MeshComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

ezSelectionContext::ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bSelectOnMouseUp = false;

  SetOwner(pOwnerWindow, pOwnerView);
}

ezEditorInut ezSelectionContext::DoMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bSelectOnMouseUp = false;

    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = GetOwnerWindow()->GetDocument()->FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<ezGizmoHandle>())
        {
          ezGizmoHandle* pGizmoHandle = static_cast<ezGizmoHandle*>(pSO);
          ezGizmo* pGizmo = pGizmoHandle->GetOwnerGizmo();

          if (pGizmo)
          {
            pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_Viewport);
            return pGizmo->MousePressEvent(e);
          }
        }
      }
    }

    m_bSelectOnMouseUp = true;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  auto* pDocument = GetOwnerWindow()->GetDocument();

  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    if (e->modifiers() & Qt::KeyboardModifier::ControlModifier)
    {
      const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

      OpenPickedMaterial(res);
    }
  }

  if (e->button() == Qt::MouseButton::LeftButton && m_bSelectOnMouseUp)
  {
    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;

    if (e->modifiers() & Qt::KeyboardModifier::AltModifier)
    {
      if (res.m_PickedComponent.IsValid())
      {
        const ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);

        if (bToggle)
          pDocument->GetSelectionManager()->ToggleObject(pObject);
        else
          pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }
    else
    {
      if (res.m_PickedObject.IsValid())
      {
        const ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);

        if (bToggle)
          pDocument->GetSelectionManager()->ToggleObject(pObject);
        else
          pDocument->GetSelectionManager()->SetSelection(pObject);
      }
    }

    m_bSelectOnMouseUp = false;

    // we handled the mouse click event
    // but this is it, we don't stay active
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}


void ezSelectionContext::OpenPickedMaterial(const ezObjectPickingResult& res) const
{
  if (!res.m_PickedComponent.IsValid())
    return;

  auto* pDocument = GetOwnerWindow()->GetDocument();

  const ezDocumentObject* pMeshComponent = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);

  // check that we actually picked a mesh component
  if (!pMeshComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezMeshComponent>())
    return;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((ezInt32)res.m_uiPartIndex < pMeshComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const ezVariant varMatGuid = pMeshComponent->GetTypeAccessor().GetValue("Materials", res.m_uiPartIndex);

    // if it were anything else than a string that would be weird
    EZ_ASSERT_DEV(varMatGuid.IsA<ezString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<ezString>())
    {
      if (TryOpenMaterial(varMatGuid.Get<ezString>()))
        return;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const ezVariant varMeshGuid = pMeshComponent->GetTypeAccessor().GetValue("Mesh");

  EZ_ASSERT_DEV(varMeshGuid.IsA<ezString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<ezString>())
    return;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!ezConversionUtils::IsStringUuid(varMeshGuid.Get<ezString>()))
    return;

  const ezUuid meshGuid = ezConversionUtils::ConvertStringToUuid(varMeshGuid.Get<ezString>());

  const ezAssetInfo* pAssetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(meshGuid);

  // unknown mesh asset
  if (!pAssetInfo)
    return;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  ezMeshAssetDocument* pMeshDoc = static_cast<ezMeshAssetDocument*>(ezQtEditorApp::GetSingleton()->OpenDocumentImmediate(pAssetInfo->m_sAbsolutePath, false, false));

  if (!pMeshDoc)
    return;

  // if we are outside the stored index, tough luck
  if (res.m_uiPartIndex < pMeshDoc->GetProperties()->m_Slots.GetCount())
  {
    TryOpenMaterial(pMeshDoc->GetProperties()->m_Slots[res.m_uiPartIndex].m_sResource);
  }

  // make sure to close the document again, if we were the ones to open it
  // otherwise keep it open
  if (!pMeshDoc->HasWindowBeenRequested())
    pMeshDoc->GetDocumentManager()->CloseDocument(pMeshDoc);
}



bool ezSelectionContext::TryOpenMaterial(const ezString& sMatRef) const
{
  const ezAssetInfo* pAssetInfo = nullptr;

  if (ezConversionUtils::IsStringUuid(sMatRef))
  {
    ezUuid matGuid;
    matGuid = ezConversionUtils::ConvertStringToUuid(sMatRef);

    pAssetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way either
    // left as an exercise for whoever needs non-asset references
    pAssetInfo = ezAssetCurator::GetSingleton()->FindAssetInfo(sMatRef);
  }

  if (pAssetInfo)
  {
    ezQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath);
    return true;
  }

  return false;
}

ezEditorInut ezSelectionContext::DoMouseMoveEvent(QMouseEvent* e)
{
  ezViewHighlightMsgToEngine msg;

  {
    const ezObjectPickingResult& res = GetOwnerWindow()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedComponent.IsValid())
    {
      const ezSceneDocument* pScene = static_cast<const ezSceneDocument*>(GetOwnerWindow()->GetDocument());

      pScene->SetPickingResult(res);
    }

    if (res.m_PickedComponent.IsValid())
      msg.m_HighlightObject = res.m_PickedComponent;
    else if (res.m_PickedOther.IsValid())
      msg.m_HighlightObject = res.m_PickedOther;
    else
      msg.m_HighlightObject = res.m_PickedObject;
  }

  msg.SendHighlightObjectMessage(GetOwnerWindow()->GetEditorEngineConnection());

  // we only updated the highlight, so others may do additional stuff, if they like
  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::DoKeyPressEvent(QKeyEvent* e)
{
  /// \todo Handle the current cursor (icon) across all active input contexts

  if (e->key() == Qt::Key_Delete)
  {
    GetOwnerWindow()->GetDocument()->DeleteSelectedObjects();
    return ezEditorInut::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_Escape)
  {
    GetOwnerWindow()->GetDocument()->GetSelectionManager()->Clear();
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}

ezEditorInut ezSelectionContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  return ezEditorInut::MayBeHandledByOthers;
}



