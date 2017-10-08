#include <PCH.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Logging/Log.h>
#include <QKeyEvent>
#include <RendererCore/Meshes/MeshComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Utilities/GraphicsUtils.h>

ezSelectionContext::ezSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera)
{
  m_pCamera = pCamera;

  SetOwner(pOwnerWindow, pOwnerView);

  m_MarqueeGizmo.Configure(nullptr, ezEngineGizmoHandleType::LineBox, ezColor::CadetBlue, false, true, false, true);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_MarqueeGizmo);
}

ezEditorInut ezSelectionContext::DoMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

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

    m_Mode = Mode::Single;

    if (e->modifiers().testFlag(Qt::ShiftModifier))
    {
      m_uiMarqueeID += 23;
      m_vMarqueeStartPos.Set(e->pos().x(), e->pos().y(), 0.01f);

      // only shift -> add, shift AND control -> remove
      m_Mode = e->modifiers().testFlag(Qt::ControlModifier) ? Mode::MarqueeRemove : Mode::MarqueeAdd;
      MakeActiveInputContext();

      if (m_Mode == Mode::MarqueeAdd)
        m_MarqueeGizmo.SetColor(ezColor::LightSkyBlue);
      else
        m_MarqueeGizmo.SetColor(ezColor::PaleVioletRed);

      return ezEditorInut::WasExclusivelyHandled;
    }
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
      const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      OpenPickedMaterial(res);
    }
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    if (m_Mode == Mode::Single)
    {
      const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;
      const bool bDirect = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;

      if (res.m_PickedObject.IsValid())
      {
        const ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);

        if (bToggle)
          pDocument->GetSelectionManager()->ToggleObject(determineObjectToSelect(pObject, true, bDirect));
        else
          pDocument->GetSelectionManager()->SetSelection(determineObjectToSelect(pObject, false, bDirect));
      }

      DoFocusLost(false);

      // we handled the mouse click event
      // but this is it, we don't stay active
      return ezEditorInut::WasExclusivelyHandled;
    }

    if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
    {
      DoFocusLost(false);
      return ezEditorInut::WasExclusivelyHandled;
    }
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

  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  ezMeshAssetDocument* pMeshDoc = static_cast<ezMeshAssetDocument*>(ezQtEditorApp::GetSingleton()->OpenDocumentImmediate(pSubAsset->m_pAssetInfo->m_sAbsolutePath, false, false));

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
  ezAssetCurator::ezLockedSubAsset pSubAsset;

  if (ezConversionUtils::IsStringUuid(sMatRef))
  {
    ezUuid matGuid;
    matGuid = ezConversionUtils::ConvertStringToUuid(sMatRef);

    pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(matGuid);
  }
  else
  {
    // I think this is even wrong, either the string is a GUID, or it is not an asset at all, in which case we cannot find it this way either
    // left as an exercise for whoever needs non-asset references
    pSubAsset = ezAssetCurator::GetSingleton()->FindSubAsset(sMatRef);
  }

  if (pSubAsset)
  {
    ezQtEditorApp::GetSingleton()->OpenDocument(pSubAsset->m_pAssetInfo->m_sAbsolutePath);
    return true;
  }

  return false;
}

ezEditorInut ezSelectionContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (IsActiveInputContext() && (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove))
  {
    ezVec2I32 curPos;
    curPos.Set(e->pos().x(), e->pos().y());

    ezMat4 mView = m_pCamera->GetViewMatrix();
    ezMat4 mProj;
    m_pCamera->GetProjectionMatrix((float)m_Viewport.x / (float)m_Viewport.y, mProj);
    ezMat4 mViewProj = mProj * mView;
    ezMat4 mInvViewProj = mViewProj.GetInverse();

    const ezVec3 vMousePos(e->pos().x(), e->pos().y(), 0.01f);

    const ezVec3 vScreenSpacePos0(vMousePos.x, m_Viewport.y - vMousePos.y, vMousePos.z);
    const ezVec3 vScreenSpacePos1(m_vMarqueeStartPos.x, m_Viewport.y - m_vMarqueeStartPos.y, m_vMarqueeStartPos.z);

    ezVec3 vPosOnNearPlane0, vRayDir0;
    ezVec3 vPosOnNearPlane1, vRayDir1;
    ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, vScreenSpacePos0, vPosOnNearPlane0, &vRayDir0);
    ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_Viewport.x, m_Viewport.y, vScreenSpacePos1, vPosOnNearPlane1, &vRayDir1);

    ezTransform t;
    t.SetIdentity();
    t.m_vPosition = ezMath::Lerp(vPosOnNearPlane0, vPosOnNearPlane1, 0.5f);
    t.m_qRotation.SetFromMat3(m_pCamera->GetViewMatrix().GetRotationalPart());

    // box coordinates in screen space
    ezVec3 vBoxPosSS0 = t.m_qRotation * vPosOnNearPlane0;
    ezVec3 vBoxPosSS1 = t.m_qRotation * vPosOnNearPlane1;

    t.m_qRotation = -t.m_qRotation;

    t.m_vScale.x = ezMath::Abs(vBoxPosSS0.x - vBoxPosSS1.x);
    t.m_vScale.y = ezMath::Abs(vBoxPosSS0.y - vBoxPosSS1.y);
    t.m_vScale.z = 0.0f;

    m_MarqueeGizmo.SetTransformation(t);
    m_MarqueeGizmo.SetVisible(true);

    {
      ezViewMarqueePickingMsgToEngine msg;
      msg.m_uiViewID = GetOwnerView()->GetViewID();
      msg.m_uiPickPosX0 = (ezUInt16)m_vMarqueeStartPos.x;
      msg.m_uiPickPosY0 = (ezUInt16)m_vMarqueeStartPos.y;
      msg.m_uiPickPosX1 = (ezUInt16)(e->pos().x());
      msg.m_uiPickPosY1 = (ezUInt16)(e->pos().y());
      msg.m_uiWhatToDo = (m_Mode == Mode::MarqueeAdd) ? 1 : 2;
      msg.m_uiActionIdentifier = m_uiMarqueeID;

      GetOwnerView()->GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
    }

    return ezEditorInut::WasExclusivelyHandled;
  }
  else
  {
    ezViewHighlightMsgToEngine msg;

    {
      const ezObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      if (res.m_PickedComponent.IsValid())
        msg.m_HighlightObject = res.m_PickedComponent;
      else if (res.m_PickedOther.IsValid())
        msg.m_HighlightObject = res.m_PickedOther;
      else
        msg.m_HighlightObject = res.m_PickedObject;
    }

    GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

    // we only updated the highlight, so others may do additional stuff, if they like
    return ezEditorInut::MayBeHandledByOthers;
  }
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

static const bool IsInSelection(const ezDeque<const ezDocumentObject*>& selection, const ezDocumentObject* pObject, const ezDocumentObject*& out_ParentInSelection, const ezDocumentObject*& out_ParentChild, const ezDocumentObject* pRootObject)
{
  if (pObject == pRootObject)
    return false;

  ezUInt32 index = selection.IndexOf(pObject);
  if (index != ezInvalidIndex)
  {
    out_ParentInSelection = pObject;
    return true;
  }

  const ezDocumentObject* pParent = pObject->GetParent();

  if (IsInSelection(selection, pParent, out_ParentInSelection, out_ParentChild, pRootObject))
  {
    if (out_ParentChild == nullptr)
      out_ParentChild = pObject;

    return true;
  }

  return false;
}

static const ezDocumentObject* GetTopMostParent(const ezDocumentObject* pObject, const ezDocumentObject* pRootObject)
{
  const ezDocumentObject* pParent = pObject;

  while (pParent->GetParent() != pRootObject)
    pParent = pParent->GetParent();

  return pParent;
}

const ezDocumentObject* ezSelectionContext::determineObjectToSelect(const ezDocumentObject* pickedObject, bool bToggle, bool bDirect) const
{
  auto* pDocument = GetOwnerWindow()->GetDocument();
  const ezDeque<const ezDocumentObject*> sel = pDocument->GetSelectionManager()->GetSelection();

  const ezDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject();

  const ezDocumentObject* pParentInSelection = nullptr;
  const ezDocumentObject* pParentChild = nullptr;

  if (!IsInSelection(sel, pickedObject, pParentInSelection, pParentChild, pRootObject))
  {
    if (bDirect)
      return pickedObject;

    return GetTopMostParent(pickedObject, pRootObject);
  }
  else
  {
    if (bToggle)
    {
      // always toggle the object that is already in the selection
      return pParentInSelection;
    }

    if (bDirect)
      return pickedObject;

    if (sel.GetCount() > 1)
    {
      // multi-selection, but no toggle, so we are about to set the selection
      // -> always use the top-level parent in this case
      return GetTopMostParent(pickedObject, pRootObject);
    }

    if (pParentInSelection == pickedObject)
    {
      // object itself is in the selection
      return pickedObject;
    }

    if (pParentChild == nullptr)
    {
      return pParentInSelection;
    }

    return pParentChild;
  }
}

void ezSelectionContext::DoFocusLost(bool bCancel)
{
  ezEditorInputContext::DoFocusLost(bCancel);

  m_Mode = Mode::None;
  m_MarqueeGizmo.SetVisible(false);

  if (IsActiveInputContext())
    MakeActiveInputContext(false);
}


