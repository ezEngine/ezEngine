#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetWindow.moc.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>

ezCollisionMeshAssetDocumentWindow::ezCollisionMeshAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "CollisionMeshAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "CollisionMeshAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("CollisionMeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("CollisionMeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Collision Mesh Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezCollisionMeshAssetDocument*>(pDocument);

  m_pLabelInfo = new QLabel(this);
  setCentralWidget(m_pLabelInfo);

  m_pLabelInfo->setText("<Mesh Information>");

  FinishWindowCreation();
}

