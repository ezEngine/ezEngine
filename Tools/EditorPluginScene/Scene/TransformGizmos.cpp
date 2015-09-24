#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <QTimer>
#include <QPushButton>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <qlayout.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Core/World/GameObject.h>
#include <QKeyEvent>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>



void ezSceneDocumentWindow::UpdateGizmoVisibility()
{
  ezSceneDocument* pSceneDoc = static_cast<ezSceneDocument*>(GetDocument());

  bool bGizmoVisible[4] = { false, false, false, false };

  if (pSceneDoc->GetSelectionManager()->GetSelection().IsEmpty() || pSceneDoc->GetActiveGizmo() == ActiveGizmo::None)
    goto done;

  if (!pSceneDoc->GetSelectionManager()->GetSelection().PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    goto done;

  switch (pSceneDoc->GetActiveGizmo())
  {
  case ActiveGizmo::Translate:
    bGizmoVisible[0] = true;
    break;
  case ActiveGizmo::Rotate:
    bGizmoVisible[1] = true;
    break;
  case ActiveGizmo::Scale:
    bGizmoVisible[2] = true;
    break;
  case ActiveGizmo::DragToPosition:
    bGizmoVisible[3] = true;
    break;
  }

  UpdateGizmoPosition();

done:

  m_TranslateGizmo.SetVisible(bGizmoVisible[0]);
  m_RotateGizmo.SetVisible(bGizmoVisible[1]);
  m_ScaleGizmo.SetVisible(bGizmoVisible[2]);
  m_DragToPosGizmo.SetVisible(bGizmoVisible[3]);

}


void ezSceneDocumentWindow::UpdateGizmoSelectionList()
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  m_GizmoSelection.Clear();

  auto hType = ezGetStaticRTTI<ezGameObject>();

  auto pSelMan = GetDocument()->GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (ezUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    SelectedGO sgo;
    sgo.m_pObject = Selection[sel];
    sgo.m_GlobalTransform = GetSceneDocument()->GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();

    m_GizmoSelection.PushBack(sgo);
  }
}

void ezSceneDocumentWindow::UpdateGizmoPosition()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezGameObject>())
  {
    const ezTransform tGlobal = GetSceneDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const ezVec3 vPivotPoint = tGlobal.m_Rotation * ezVec3::ZeroVector();// LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

    ezMat4 mt;

    if (GetSceneDocument()->GetGizmoWorldSpace())
    {
      mt.SetTranslationMatrix(tGlobal.m_vPosition + vPivotPoint);
    }
    else
    {
      mt = ezMat4(tGlobal.m_Rotation, tGlobal.m_vPosition + vPivotPoint);
      mt.SetScalingFactors(ezVec3(1.0f));
    }

    m_TranslateGizmo.SetTransformation(mt);
    m_RotateGizmo.SetTransformation(mt);
    m_ScaleGizmo.SetTransformation(mt);
    m_DragToPosGizmo.SetTransformation(mt);
  }

}


void ezSceneDocumentWindow::TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e)
{
  switch (e.m_Type)
  {
  case ezGizmoBase::BaseEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;

      // duplicate the object when shift is held while dragging the item
      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier)
      {
        m_bMergeTransactions = true;
        GetSceneDocument()->DuplicateSelection();
      }

      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      {
        m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);
      }

      UpdateGizmoSelectionList();

      GetDocument()->GetCommandHistory()->BeginTemporaryCommands();
    }
    break;

  case ezGizmoBase::BaseEvent::Type::EndInteractions:
    {
      GetDocument()->GetCommandHistory()->FinishTemporaryCommands();

      m_GizmoSelection.Clear();

      if (m_bMergeTransactions)
        GetDocument()->GetCommandHistory()->MergeLastTwoTransactions();
    }
    break;

  case ezGizmoBase::BaseEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      GetDocument()->GetCommandHistory()->StartTransaction();

      auto pScene = GetSceneDocument();
      ezTransform tNew;

      bool bCancel = false;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        const ezVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          pScene->SetGlobalTransform(obj.m_pObject, tNew);
        }

        if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);

          auto* pFocusedView = GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(m_TranslateGizmo.GetTranslationDiff());
          }
        }
        else
        {
          m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::ScreenProjection);
        }
      }

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const ezQuat qRotation = m_RotateGizmo.GetRotationResult();
        const ezVec3 vPivot = m_RotateGizmo.GetTransformation().GetTranslationVector();

        const ezMat3 mRot = qRotation.GetAsMat3();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_Rotation = mRot * obj.m_GlobalTransform.m_Rotation;
          tNew.m_vPosition = vPivot + mRot * (obj.m_GlobalTransform.m_vPosition - vPivot);

          pScene->SetGlobalTransform(obj.m_pObject, tNew);
        }
      }

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        ezSetObjectPropertyCommand cmd;
        cmd.SetPropertyPath("LocalScaling");

        const ezVec3 vScale = m_ScaleGizmo.GetScalingResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          cmd.m_Object = obj.m_pObject->GetGuid();
          cmd.m_NewValue = obj.m_vLocalScaling.CompMult(vScale);

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        const ezVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();
        const ezQuat qRot = m_DragToPosGizmo.GetRotationResult();

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;
          tNew.m_Rotation = qRot.GetAsMat3(); /// \todo preserve scaling ?

          pScene->SetGlobalTransform(obj.m_pObject, tNew);
        }
      }

      if (bCancel)
        GetDocument()->GetCommandHistory()->CancelTransaction();
      else
        GetDocument()->GetCommandHistory()->FinishTransaction();

      m_bInGizmoInteraction = false;
    }
    break;
  }

}



