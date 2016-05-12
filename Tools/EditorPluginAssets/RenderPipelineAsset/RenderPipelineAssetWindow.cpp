#include <PCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>

#include <QLabel>
#include <QLayout>


ezRenderPipelineAssetDocumentWindow::ezRenderPipelineAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "RenderPipelineAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "RenderPipelineAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("RenderPipelineAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  ezQtNodeScene* m_pScene = new ezQtNodeScene(this);
  m_pScene->SetDocumentNodeManager(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));
  ezQtNodeView* m_pView = new ezQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("RenderPipelineAssetDockWidget");
    pPropertyPanel->setWindowTitle("Render Pipeline Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  FinishWindowCreation();
}

ezRenderPipelineAssetDocumentWindow::~ezRenderPipelineAssetDocumentWindow()
{
}
