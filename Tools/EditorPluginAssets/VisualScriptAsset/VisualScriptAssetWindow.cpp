#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>

#include <QLabel>
#include <QLayout>

ezQtVisualScriptAssetDocumentWindow::ezQtVisualScriptAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new ezQtVisualScriptAssetScene(this);
  m_pScene->SetDocumentNodeManager(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));
  m_pView = new ezQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  static_cast<ezVisualScriptAssetDocument*>(pDocument)->m_ActivityEvents.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));

  FinishWindowCreation();
}

ezQtVisualScriptAssetDocumentWindow::~ezQtVisualScriptAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    static_cast<ezVisualScriptAssetDocument*>(GetDocument())->m_ActivityEvents.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));
  }
}

void ezQtVisualScriptAssetDocumentWindow::PickDebugTarget()
{
  // Currently cannot be executed with this != nullptr, because actions don't have a proper window pointer
  int i = 0;
}

