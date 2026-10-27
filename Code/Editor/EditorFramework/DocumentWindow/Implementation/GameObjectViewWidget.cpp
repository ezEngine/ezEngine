#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

ezQtGameObjectViewWidget::ezQtGameObjectViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this);
  m_pOrthoGizmoContext = EZ_DEFAULT_NEW(ezOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pCameraMoveContext->LoadState();

  SetShowOrientationIndicator(true);

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pOrthoGizmoContext);
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

ezQtGameObjectViewWidget::~ezQtGameObjectViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrthoGizmoContext);
  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pCameraMoveContext);
}

void ezQtGameObjectViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(ezVec2I32(width(), height()));

  ezQtEngineViewWidget::SyncToEngine();
}

void ezQtGameObjectViewWidget::GetOrbitStart(ezVec3& out_vOrbitPoint, ezVec3& out_vDirection, ezVec3& out_vUp) const
{
  if (GetCameraOrbitTarget(out_vOrbitPoint, out_vDirection, out_vUp))
    return;

  const ezCamera& cam = m_pViewConfig->m_Camera;
  out_vOrbitPoint = m_pCameraMoveContext->GetOrbitPoint();
  out_vDirection = cam.GetCenterDirForwards();
  out_vUp = cam.GetCenterDirUp();
}

void ezQtGameObjectViewWidget::OrbitCameraTo(const ezVec3& vDirection, const ezVec3& vUp)
{
  ezVec3 vOrbitPoint, vCurDirection, vCurUp;
  GetOrbitStart(vOrbitPoint, vCurDirection, vCurUp);

  InterpolateCameraAroundPoint(vOrbitPoint, vDirection, vUp);
}

void ezQtGameObjectViewWidget::OrbitCameraBy(ezAngle yaw, ezAngle pitch)
{
  ezVec3 vOrbitPoint, vDirection, vUp;
  GetOrbitStart(vOrbitPoint, vDirection, vUp);

  const ezVec3 vRight = vUp.CrossRH(vDirection).GetNormalized();
  const ezQuat qRotation = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), yaw) * ezQuat::MakeFromAxisAndAngle(vRight, pitch);

  InterpolateCameraAroundPoint(vOrbitPoint, qRotation * vDirection, qRotation * vUp);
}

void ezQtGameObjectViewWidget::HandleMarqueePickingResult(const ezViewMarqueePickingResultMsgToEditor* pMsg)
{
  auto pSelMan = GetDocumentWindow()->GetDocument()->GetSelectionManager();
  auto pObjMan = GetDocumentWindow()->GetDocument()->GetObjectManager();

  if (m_uiLastMarqueeActionID != pMsg->m_uiActionIdentifier)
  {
    m_uiLastMarqueeActionID = pMsg->m_uiActionIdentifier;

    m_MarqueeBaseSelection.Clear();

    if (pMsg->m_uiWhatToDo == 0) // set selection
      pSelMan->Clear();

    const auto& curSel = pSelMan->GetSelection();
    for (auto pObj : curSel)
    {
      m_MarqueeBaseSelection.PushBack(pObj->GetGuid());
    }
  }

  ezDeque<const ezDocumentObject*> newSelection;

  for (ezUuid guid : m_MarqueeBaseSelection)
  {
    auto pObject = pObjMan->GetObject(guid);
    newSelection.PushBack(pObject);
  }

  for (ezUuid guid : pMsg->m_ObjectGuids)
  {
    const ezDocumentObject* pObject = pObjMan->GetObject(guid);

    if (pMsg->m_uiWhatToDo == 2) // remove from selection
    {
      // keep selection order
      newSelection.RemoveAndCopy(pObject);
    }
    else // add/set selection
    {
      if (!newSelection.Contains(pObject))
        newSelection.PushBack(pObject);
    }
  }

  pSelMan->SetSelection(newSelection);
}
