#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// ezLUTAssetActions
//////////////////////////////////////////////////////////////////////////


void ezLUTAssetActions::RegisterActions() {}

void ezLUTAssetActions::UnregisterActions() {}

void ezLUTAssetActions::MapActions(ezStringView sMapping) {}


//////////////////////////////////////////////////////////////////////////
// ezQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtLUTAssetDocumentWindow::ezQtLUTAssetDocumentWindow(ezLUTAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "LUTAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "LUTAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("LUTAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(ezVec3(0), ezVec3(0.5f), ezVec3(-2, 0, 0));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, nullptr);

    m_pDockManager->setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("LUTAssetDockWidget");
    pPropertyPanel->setWindowTitle("LUT Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator((ezAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void ezQtLUTAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtLUTAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const ezLUTAssetDocument* pDoc = static_cast<const ezLUTAssetDocument*>(GetDocument());

    ezDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMode";
    msg.m_iValue = pDoc->m_PreviewMode.GetValue();
    GetEditorEngineConnection()->SendMessage(&msg);

    ezDocumentConfigMsgToEngine msg2;
    msg2.m_sWhatToDo = "SliceCoordinate";
    msg2.m_fValue = pDoc->m_fSliceCoordinate;
    GetEditorEngineConnection()->SendMessage(&msg2);

    ezDocumentConfigMsgToEngine msg3;
    msg3.m_sWhatToDo = "OpacityMultiplier";
    msg3.m_fValue = pDoc->m_fOpacityMultiplier;
    GetEditorEngineConnection()->SendMessage(&msg3);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
