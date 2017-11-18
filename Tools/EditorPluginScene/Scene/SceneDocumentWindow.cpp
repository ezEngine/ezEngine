#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QGridLayout>
#include <QSettings>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <Core/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>


ezQtSceneDocumentWindow::ezQtSceneDocumentWindow(ezSceneDocument* pDocument)
  : ezQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](ezQtEngineDocumentWindow* pWindow, ezEngineViewConfig* pConfig) -> ezQtEngineViewWidget*
  {
    ezQtSceneViewWidget* pWidget = new ezQtSceneViewWidget(nullptr, static_cast<ezQtSceneDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new ezQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate([this](ezGameObjectEditTool* pTool)
  {
    pTool->ConfigureTool(static_cast<ezGameObjectDocument*>(GetDocument()), this, this);
  });

  setCentralWidget(m_pQuadViewWidget);

  SetTargetFramerate(25);

  {
    // Menu Bar
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::GameObjectEventHandler, this));

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezQtDocumentPanel* pPanelTree = new ezQtScenegraphPanel(this, static_cast<ezSceneDocument*>(pDocument));
    pPanelTree->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  FinishWindowCreation();
}

ezQtSceneDocumentWindow::~ezQtSceneDocumentWindow()
{
  GetSceneDocument()->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::GameObjectEventHandler, this));
}

ezSceneDocument* ezQtSceneDocumentWindow::GetSceneDocument() const
{
  return static_cast<ezSceneDocument*>(GetDocument());
}

void ezQtSceneDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


ezObjectAccessorBase* ezQtSceneDocumentWindow::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool ezQtSceneDocumentWindow::CanDuplicateSelection() const
{
  return true;
}

void ezQtSceneDocumentWindow::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void ezQtSceneDocumentWindow::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = ezSnapProvider::GetTranslationSnapValue();

  if (fSnap == 0.0f)
    return;

  const ezDeque<const ezDocumentObject*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
  if (selection.IsEmpty())
    return;

  const auto& pivotObj = selection.PeekBack();

  ezVec3 vPivotSnapOffset;

  if (!bSnapEachObject)
  {
    // if we snap by the pivot object only, the last selected object must be a valid game object
    if (!pivotObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      return;

    const ezVec3 vPivotPos = GetSceneDocument()->GetGlobalTransform(pivotObj).m_vPosition;
    ezVec3 vSnappedPos = vPivotPos;
    ezSnapProvider::SnapTranslation(vSnappedPos);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  ezDeque<ezSelectedGameObject> gizmoSelection;
  GetGameObjectDocument()->ComputeTopLevelSelectedGameObjects(gizmoSelection);

  if (gizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (ezUInt32 sel = 0; sel < gizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = gizmoSelection[sel];

    ezTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition = obj.m_GlobalTransform.m_vPosition;
      ezSnapProvider::SnapTranslation(vSnappedPos.m_vPosition);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos, TransformationChanges::Translation);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  gizmoSelection.Clear();
}

void ezQtSceneDocumentWindow::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
    // Focus is done by ezQtGameObjectDocumentWindow
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    break;

  case ezGameObjectEvent::Type::TriggerFocusOnSelection_All:
    // Focus is done by ezQtGameObjectDocumentWindow
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    break;

  case ezGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid:
    SnapSelectionToPosition(false);
    break;

  case ezGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid:
    SnapSelectionToPosition(true);
    break;
  }
}

void ezQtSceneDocumentWindow::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  ezSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSceneDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSimulationSettingsMsgToEngine msg;
    auto pSceneDoc = GetSceneDocument();
    msg.m_bSimulateWorld = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed = pSceneDoc->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    ezGridSettingsMsgToEngine msg = GetGridSettings();
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    ezGlobalSettingsMsgToEngine msg = GetGlobalSettings();
    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    ezWorldSettingsMsgToEngine msg = GetWorldSettings();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  GetGameObjectDocument()->SendObjectSelection();

  auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(pView == pHoveredView);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtSceneDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  ezQtGameObjectDocumentWindow::ProcessMessageEventHandler(pMsg);
}



