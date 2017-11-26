#include <PCH.h>
#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectGizmoEditTool, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezGameObjectGizmoEditTool::ezGameObjectGizmoEditTool()
{
}

ezGameObjectGizmoEditTool::~ezGameObjectGizmoEditTool()
{
  // currently edit tools are only deallocated when a document is closed, so event handlers do not need to be removed
  // additionally, the window is already destroyed at that time, so we actually should not try to remove those handlers

  //GetDocument()->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::GameObjectEventHandler, this));
  //GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
  //GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
  //ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
  //GetWindow()->m_EngineWindowEvent.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::EngineWindowEventHandler, this));
  //GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::ObjectStructureEventHandler, this));
}

void ezGameObjectGizmoEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::GameObjectEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
  GetWindow()->m_EngineWindowEvent.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::EngineWindowEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::ObjectStructureEventHandler, this));

  // subscribe to all views that already exist
  for (ezQtEngineViewWidget* pView : GetWindow()->GetViewWidgets())
  {
    if (ezQtGameObjectViewWidget* pViewWidget = qobject_cast<ezQtGameObjectViewWidget*>(pView))
    {
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
    }
  }
}

void ezGameObjectGizmoEditTool::UpdateGizmoSelectionList()
{
  GetDocument()->ComputeTopLevelSelectedGameObjects(m_GizmoSelection);
}

void ezGameObjectGizmoEditTool::UpdateGizmoVisibleState()
{
  bool isVisible = false;

  if (IsActive())
  {
    ezGameObjectDocument* pDocument = GetDocument();

    const auto& selection = pDocument->GetSelectionManager()->GetSelection();

    if (selection.IsEmpty() || !selection.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      goto done;

    isVisible = true;
    UpdateGizmoTransformation();
  }

done:
  ApplyGizmoVisibleState(isVisible);
}

void ezGameObjectGizmoEditTool::UpdateGizmoTransformation()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezGameObject>())
  {
    const ezTransform tGlobal = GetDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const ezVec3 vPivotPoint = tGlobal.m_qRotation * ezVec3::ZeroVector();// LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

    ezTransform mt;
    mt.SetIdentity();

    if (GetDocument()->GetGizmoWorldSpace() && GetSupportedSpaces() != ezEditToolSupportedSpaces::LocalSpaceOnly)
    {
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }
    else
    {
      mt.m_qRotation = tGlobal.m_qRotation;
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }

    ApplyGizmoTransformation(mt);
  }
}

void ezGameObjectGizmoEditTool::UpdateManipulatorVisibility()
{
  ezManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);
}

void ezGameObjectGizmoEditTool::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::ActiveEditToolChanged:
    UpdateGizmoVisibleState();
    UpdateManipulatorVisibility();
    break;
  }
}

void ezGameObjectGizmoEditTool::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
  case ezCommandHistoryEvent::Type::UndoEnded:
  case ezCommandHistoryEvent::Type::RedoEnded:
  case ezCommandHistoryEvent::Type::TransactionEnded:
  case ezCommandHistoryEvent::Type::TransactionCanceled:
    UpdateGizmoVisibleState();
    break;
  }
}

void ezGameObjectGizmoEditTool::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManagerEvent::Type::SelectionCleared:
    m_GizmoSelection.Clear();
    UpdateGizmoVisibleState();
    break;

  case ezSelectionManagerEvent::Type::SelectionSet:
  case ezSelectionManagerEvent::Type::ObjectAdded:
    EZ_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");
    UpdateGizmoVisibleState();
    break;
  }
}

void ezGameObjectGizmoEditTool::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void ezGameObjectGizmoEditTool::EngineWindowEventHandler(const ezEngineWindowEvent& e)
{
  if (ezQtGameObjectViewWidget* pViewWidget = qobject_cast<ezQtGameObjectViewWidget*>(e.m_pView))
  {
    switch (e.m_Type)
    {
    case ezEngineWindowEvent::Type::ViewCreated:
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
      break;
    }
  }
}

void ezGameObjectGizmoEditTool::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (!IsActive() || m_bInGizmoInteraction)
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    UpdateGizmoVisibleState();
    break;
  }
}

void ezGameObjectGizmoEditTool::TransformationGizmoEventHandler(const ezGizmoEvent& e)
{
  if (!IsActive())
    return;

  ezObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();

  switch (e.m_Type)
  {
  case ezGizmoEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;

      TransformationGizmoEventHandlerImpl(e);

      UpdateGizmoSelectionList();

      pAccessor->BeginTemporaryCommands("Transform Object");
    }
    break;

  case ezGizmoEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      pAccessor->StartTransaction("Transform Object");

      TransformationGizmoEventHandlerImpl(e);

      m_bInGizmoInteraction = false;
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
  }

}
