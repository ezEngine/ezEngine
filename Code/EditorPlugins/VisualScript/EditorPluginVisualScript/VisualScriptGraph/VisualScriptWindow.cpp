#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>


ezQtVisualScriptWindow::ezQtVisualScriptWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    m_pScene = new ezQtVisualScriptNodeScene(this);
    m_pScene->InitScene(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));

    m_pView = new ezQtNodeView(this);
    m_pView->SetScene(m_pScene);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("VisualScriptView");
    pCentral->setWindowTitle("Script");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
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

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptWindow::SelectionEventHandler, this));

  FinishWindowCreation();

  SelectionEventHandler(ezSelectionManagerEvent());
}

ezQtVisualScriptWindow::~ezQtVisualScriptWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezQtVisualScriptWindow::SelectionEventHandler, this));
  }
}

void ezQtVisualScriptWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1,
      [this]()
      {
        auto pDocument = GetDocument();
        auto pSelectionManager = pDocument->GetSelectionManager();

        // Check again if the selection is empty. This could have changed due to the delayed execution.
        if (pSelectionManager->IsSelectionEmpty())
        {
          pSelectionManager->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
        }
      });
  }
}
