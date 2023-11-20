#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <GuiFoundation/VisualGraph/View.moc.h>
#include <QVBoxLayout>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

ezQtAiControllerAssetDocumentWindow::ezQtAiControllerAssetDocumentWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "AiControllerAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "AiControllerAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AiControllerAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new ezQtVisualGraphScene(this);
    m_pScene->InitScene(static_cast<const ezVisualGraphObjectManager*>(pDocument->GetObjectManager()));

    m_pView = new ezQtVisualGraphView(this);
    m_pView->SetScene(m_pScene);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("AiControllerView");
    pCentral->setWindowTitle("AI Controller");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("AiControllerAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
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
  }

  FinishWindowCreation();
}

ezQtAiControllerAssetDocumentWindow::~ezQtAiControllerAssetDocumentWindow() = default;
