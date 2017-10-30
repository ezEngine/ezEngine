
#include <PCH.h>
#include <EditorFramework/DocumentWindow/GameObjectGizmoHandler.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezGameObjectGizmoHandler::ezGameObjectGizmoHandler(ezGameObjectDocument* pDocument,
  ezQtGameObjectDocumentWindow* pWindow, ezGameObjectGizmoInterface* pInterface)
  : m_pDocument(pDocument)
  , m_pWindow(pWindow)
  , m_pInterface(pInterface)
{
  // TODO: (works but..) give the gizmo the proper view? remove the view from the input context altogether?
  m_TranslateGizmo.SetOwner(pWindow, nullptr);
  m_RotateGizmo.SetOwner(pWindow, nullptr);
  m_ScaleGizmo.SetOwner(pWindow, nullptr);
  m_DragToPosGizmo.SetOwner(pWindow, nullptr);

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));

  m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::ObjectStructureEventHandler, this));
  m_pDocument->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::CommandHistoryEventHandler, this));
  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::SelectionManagerEventHandler, this));
  m_pDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::GameObjectEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::ManipulatorManagerEventHandler, this));
  pWindow->m_EngineWindowEvent.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::EngineWindowEventHandler, this));
}

ezGameObjectGizmoHandler::~ezGameObjectGizmoHandler()
{
  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));

  m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::ObjectStructureEventHandler, this));
  m_pDocument->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::CommandHistoryEventHandler, this));
  m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::SelectionManagerEventHandler, this));
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::GameObjectEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::ManipulatorManagerEventHandler, this));
  m_pWindow->m_EngineWindowEvent.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::EngineWindowEventHandler, this));
}

ezGameObjectDocument* ezGameObjectGizmoHandler::GetDocument() const
{
  return m_pDocument;
}

ezDeque<ezGameObjectGizmoHandler::SelectedGO> ezGameObjectGizmoHandler::GetSelectedGizmoObjects()
{
  UpdateGizmoSelectionList();
  return m_GizmoSelection;
}

void ezGameObjectGizmoHandler::UpdateManipulatorVisibility()
{
  ezManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveGizmo() != ActiveGizmo::None);
}

void ezGameObjectGizmoHandler::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (m_bInGizmoInteraction)
    return;

  if (!m_TranslateGizmo.IsVisible() && !m_RotateGizmo.IsVisible() && !m_ScaleGizmo.IsVisible() && !m_DragToPosGizmo.IsVisible())
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezGameObjectGizmoHandler::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::ActiveGizmoChanged:
    UpdateGizmoVisibility();
    //if (!m_bIgnoreGizmoChangedEvent)
    {
      UpdateManipulatorVisibility();
    }
    break;
  }
}

void ezGameObjectGizmoHandler::EngineWindowEventHandler(const ezEngineWindowEvent& e)
{
  if (ezQtGameObjectViewWidget* pViewWidget = qobject_cast<ezQtGameObjectViewWidget*>(e.m_pView))
  {
    switch (e.m_Type)
    {
    case ezEngineWindowEvent::Type::ViewCreated:
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
      break;
    case ezEngineWindowEvent::Type::ViewDestroyed:
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoHandler::TransformationGizmoEventHandler, this));
      break;
    }
  }
}

void ezGameObjectGizmoHandler::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
  case ezCommandHistoryEvent::Type::UndoEnded:
  case ezCommandHistoryEvent::Type::RedoEnded:
  case ezCommandHistoryEvent::Type::TransactionEnded:
  case ezCommandHistoryEvent::Type::TransactionCanceled:
    {
      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezGameObjectGizmoHandler::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManagerEvent::Type::SelectionCleared:
    {
      m_GizmoSelection.Clear();
      UpdateGizmoVisibility();
    }
    break;

  case ezSelectionManagerEvent::Type::SelectionSet:
  case ezSelectionManagerEvent::Type::ObjectAdded:
    {
      EZ_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");

      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezGameObjectGizmoHandler::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveGizmo(ActiveGizmo::None);
  }
}

void ezGameObjectGizmoHandler::UpdateGizmoVisibility()
{
  ezGameObjectDocument* pDocument = GetDocument();

  bool bGizmoVisible[4] = { false, false, false, false };

  if (pDocument->GetSelectionManager()->GetSelection().IsEmpty() || pDocument->GetActiveGizmo() == ActiveGizmo::None)
    goto done;

  if (!pDocument->GetSelectionManager()->GetSelection().PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    goto done;

  switch (pDocument->GetActiveGizmo())
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

void ezGameObjectGizmoHandler::UpdateGizmoSelectionList()
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
    sgo.m_GlobalTransform = GetDocument()->GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<ezVec3>();
    sgo.m_fLocalUniformScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

    m_GizmoSelection.PushBack(sgo);
  }
}

void ezGameObjectGizmoHandler::UpdateGizmoPosition()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezGameObject>())
  {
    const ezTransform tGlobal = GetDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const ezVec3 vPivotPoint = tGlobal.m_qRotation * ezVec3::ZeroVector();// LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

    ezTransform mt;
    mt.SetIdentity();

    if (GetDocument()->GetGizmoWorldSpace())
    {
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }
    else
    {
      mt.m_qRotation = tGlobal.m_qRotation;
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }

    m_TranslateGizmo.SetTransformation(mt);
    m_RotateGizmo.SetTransformation(mt);
    m_ScaleGizmo.SetTransformation(mt);
    m_DragToPosGizmo.SetTransformation(mt);
  }
}

void ezGameObjectGizmoHandler::TransformationGizmoEventHandler(const ezGizmoEvent& e)
{
  ezObjectAccessorBase* pAccessor = m_pInterface->GetObjectAccessor();
  switch (e.m_Type)
  {
  case ezGizmoEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;
      const bool bDuplicate = m_pInterface->CanDuplicateSelection() && QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);

      // duplicate the object when shift is held while dragging the item
      if ((e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo == &m_RotateGizmo || e.m_pGizmo == &m_DragToPosGizmo) && bDuplicate)
      {
        m_bMergeTransactions = true;
        m_pInterface->DuplicateSelection();
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>())
      {
        if (m_TranslateGizmo.IsVisible() && bDuplicate)
        {
          m_bMergeTransactions = true;
          m_pInterface->DuplicateSelection();
        }
      }

      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      {
        m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);
      }

      UpdateGizmoSelectionList();

      pAccessor->BeginTemporaryCommands("Transform Object");
    }
    break;

  case ezGizmoEvent::Type::EndInteractions:
    {
      pAccessor->FinishTemporaryCommands();
      m_GizmoSelection.Clear();

      if (m_bMergeTransactions)
        GetDocument()->GetCommandHistory()->MergeLastTwoTransactions(); //#TODO: this should be interleaved transactions
    }
    break;

  case ezGizmoEvent::Type::CancelInteractions:
    {
      pAccessor->CancelTemporaryCommands();
      m_GizmoSelection.Clear();
    }
    break;

  case ezGizmoEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      pAccessor->StartTransaction("Transform Object");

      auto pDocument = GetDocument();
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

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          m_TranslateGizmo.SetMovementMode(ezTranslateGizmo::MovementMode::MouseDiff);

          auto* pFocusedView = m_pWindow->GetFocusedViewWidget();
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
        const ezVec3 vPivot = m_RotateGizmo.GetTransformation().m_vPosition;

        for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
        }
      }

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        const ezVec3 vScale = m_ScaleGizmo.GetScalingResult();
        if (vScale.x == vScale.y && vScale.x == vScale.z)
        {
          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            float fNewScale = obj.m_fLocalUniformScaling * vScale.x;

            if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
        else
        {
          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            ezVec3 vNewScale = obj.m_vLocalScaling.CompMul(vScale);

            if (pAccessor->SetValue(obj.m_pObject, "LocalScaling", vNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
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

          if (m_DragToPosGizmo.ModifiesRotation())
          {
            tNew.m_qRotation = qRot;
          }

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation | TransformationChanges::Rotation);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<ezOrthoGizmoContext>())
      {
        const ezOrthoGizmoContext* pOrtho = static_cast<const ezOrthoGizmoContext*>(e.m_pGizmo);

        if (m_TranslateGizmo.IsVisible())
        {
          const ezVec3 vTranslate = pOrtho->GetTranslationResult();

          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];

            tNew = obj.m_GlobalTransform;
            tNew.m_vPosition += vTranslate;

            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
          }

          if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
          {
            // move the camera with the translated object

            auto* pFocusedView = m_pWindow->GetFocusedViewWidget();
            if (pFocusedView != nullptr)
            {
              pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(pOrtho->GetTranslationDiff());
            }
          }
        }

        if (m_RotateGizmo.IsVisible())
        {
          const ezQuat qRotation = pOrtho->GetRotationResult();

          //const ezVec3 vPivot(0);

          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];

            tNew = obj.m_GlobalTransform;
            tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
            //tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation);
          }
        }

        if (m_ScaleGizmo.IsVisible())
        {
          const float fScale = pOrtho->GetScalingResult();
          for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            const float fNewScale = obj.m_fLocalUniformScaling * fScale;

            if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
      }

      if (bCancel)
        pAccessor->CancelTransaction();
      else
        pAccessor->FinishTransaction();

      m_bInGizmoInteraction = false;
    }
    break;
  }
}
