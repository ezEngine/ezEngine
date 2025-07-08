#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtSceneDocumentWindow::ezQtSceneDocumentWindow(ezSceneDocument* pDocument)
  : ezQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](ezQtEngineDocumentWindow* pWindow, ezEngineViewConfig* pConfig) -> ezQtEngineViewWidget*
  {
    ezQtSceneViewWidget* pWidget = new ezQtSceneViewWidget(nullptr, static_cast<ezQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new ezQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](ezGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<ezGameObjectDocument*>(GetDocument()), this, this); });

  {
    ezQtDocumentPanel* pViewPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pViewPanel->setObjectName("ezQtDocumentPanel");
    pViewPanel->setWindowTitle("3D View");
    pViewPanel->setWidget(m_pQuadViewWidget);

    m_pDockManager->setCentralWidget(pViewPanel);
  }

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  SetTargetFramerate(pPreferences->GetMaxFramerate());

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

  // Exposed Parameters
  if (GetSceneDocument()->IsPrefab())
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPanel->setObjectName("PrefabSettingsPanel");
    pPanel->setWindowTitle("Prefab Settings");

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPanel, pDocument, false);
    ezDeque<const ezDocumentObject*> selection;
    selection.PushBack(pDocument->GetSettingsObject());
    pPropertyGrid->SetSelection(selection);
    pPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
  }

  // Properties
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");

    ezQtDocumentPanel* pPanelTree = new ezQtScenegraphPanel(GetContainerWindow()->GetDockManager(), this, static_cast<ezSceneDocument*>(pDocument));
    pPanelTree->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    EZ_VERIFY(connect(pPropertyGrid, &ezQtPropertyGridWidget::ExtendContextMenu, this, &ezQtSceneDocumentWindow::ExtendPropertyGridContextMenu), "");

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);
    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }

  FinishWindowCreation();
}

ezQtSceneDocumentWindow::~ezQtSceneDocumentWindow() = default;

ezQtSceneDocumentWindowBase::ezQtSceneDocumentWindowBase(ezSceneDocument* pDocument)
  : ezQtGameObjectDocumentWindow(pDocument)
{
  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

ezQtSceneDocumentWindowBase::~ezQtSceneDocumentWindowBase()
{
  GetSceneDocument()->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

ezSceneDocument* ezQtSceneDocumentWindowBase::GetSceneDocument() const
{
  return static_cast<ezSceneDocument*>(GetDocument());
}

void ezQtSceneDocumentWindowBase::CreateImageCapture(const char* szOutputPath)
{
  m_pQuadViewWidget->GetActiveMainViews()[0]->GetViewWidget()->TakeScreenshot(szOutputPath);
}

void ezQtSceneDocumentWindowBase::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


ezObjectAccessorBase* ezQtSceneDocumentWindowBase::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool ezQtSceneDocumentWindowBase::CanDuplicateSelection() const
{
  return true;
}

void ezQtSceneDocumentWindowBase::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void ezQtSceneDocumentWindowBase::SnapSelectionToPosition(bool bSnapEachObject)
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

  ShowTemporaryStatusBarMsg(ezFmt("Snap to Grid ({})", bSnapEachObject ? "Each Object" : "Pivot"));
}

void ezQtSceneDocumentWindowBase::GameObjectEventHandler(const ezGameObjectEvent& e)
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

    default:
      break;
  }
}

void ezQtSceneDocumentWindowBase::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  ezSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSceneDocumentWindowBase::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSimulationSettingsMsgToEngine msg;
    auto pSceneDoc = GetSceneDocument();
    msg.m_bSimulateWorld = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed = pSceneDoc->GetPauseSimulation() ? 0.0f : pSceneDoc->GetSimulationSpeed();

    if (msg.m_bSimulateWorld && pSceneDoc->GetStepSimulation())
    {
      msg.m_fSimulationSpeed = 1.0f;
      pSceneDoc->SetStepSimulation(false);
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }
  {
    ezGridSettingsMsgToEngine msg = GetGridSettings();
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
    pView->SetPickTransparent(GetGameObjectDocument()->GetPickTransparent());
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtSceneDocumentWindowBase::ExtendPropertyGridContextMenu(
  QMenu& menu, const ezHybridArray<ezPropertySelection, 8>& items, const ezAbstractProperty* pProp)
{
  if (!GetSceneDocument()->IsPrefab())
    return;

  ezUInt32 iExposed = 0;
  for (ezUInt32 i = 0; i < items.GetCount(); i++)
  {
    ezInt32 index = GetSceneDocument()->FindExposedParameter(items[i].m_pObject, pProp, items[i].m_Index);
    if (index != -1)
      iExposed++;
  }
  menu.addSeparator();
  {
    QAction* pAction = menu.addAction("Expose as Parameter");
    pAction->setEnabled(iExposed < items.GetCount());
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]()
      {
      while (true)
      {
        bool bOk = false;
        QString name = QInputDialog::getText(this, "Parameter Name", "Name:", QLineEdit::Normal, pProp->GetPropertyName(), &bOk);

        if (!bOk)
          return;

        if (!ezStringUtils::IsValidIdentifierName(name.toUtf8().data()))
        {
          ezQtUiServices::GetSingleton()->MessageBoxInformation("This name is not a valid identifier.\nAllowed characters are a-z, A-Z, "
                                                                "0-9 and _.\nWhitespace and special characters are not allowed.");
          continue; // try again
        }

        auto pAccessor = GetSceneDocument()->GetObjectAccessor();
        pAccessor->StartTransaction("Expose as Parameter");
        for (const ezPropertySelection& sel : items)
        {
          ezInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
          if (index == -1)
          {
            GetSceneDocument()->AddExposedParameter(name.toUtf8(), sel.m_pObject, pProp, sel.m_Index).LogFailure();
          }
        }
        pAccessor->FinishTransaction();
        return;
      } });
  }
  {
    QAction* pAction = menu.addAction("Remove Exposed Parameter");
    pAction->setEnabled(iExposed > 0);
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]()
      {
      auto pAccessor = GetSceneDocument()->GetObjectAccessor();
      pAccessor->StartTransaction("Remove Exposed Parameter");
      for (const ezPropertySelection& sel : items)
      {
        ezInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
        if (index != -1)
        {
          GetSceneDocument()->RemoveExposedParameter(index).LogFailure();
        }
      }
      pAccessor->FinishTransaction(); });
  }
}

void ezQtSceneDocumentWindowBase::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  ezQtGameObjectDocumentWindow::ProcessMessageEventHandler(pMsg);
}
