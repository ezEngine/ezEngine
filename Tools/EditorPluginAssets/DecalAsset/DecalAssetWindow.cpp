#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

//////////////////////////////////////////////////////////////////////////
// ezQtDecalAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtDecalAssetDocumentWindow::ezQtDecalAssetDocumentWindow(ezDecalAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "DecalAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "DecalAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("DecalAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    //m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    //m_ViewConfig.ApplyPerspectiveSetting(90);

    //m_pViewWidget = new ezQtTextureViewWidget(nullptr, this, &m_ViewConfig);

    //ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    //setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("DecalAssetDockWidget");
    pPropertyPanel->setWindowTitle("Decal Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  }

  FinishWindowCreation();
}

void ezQtDecalAssetDocumentWindow::InternalRedraw()
{
  //ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  //ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtDecalAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  //{
  //  ezSceneSettingsMsgToEngine msg;
  //  msg.m_fGizmoScale = 0;
  //  GetEditorEngineConnection()->SendMessage(&msg);
  //}

  //for (auto pView : m_ViewWidgets)
  //{
  //  pView->SetEnablePicking(false);
  //  pView->UpdateCameraInterpolation();
  //  pView->SyncToEngine();
  //}
}

