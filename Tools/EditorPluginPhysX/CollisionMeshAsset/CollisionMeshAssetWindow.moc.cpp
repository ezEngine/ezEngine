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
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentWindow::PropertyEventHandler, this));

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
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
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

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezCollisionMeshAssetDocument*>(pDocument);
  m_pAssetDoc->m_AssetEvents.AddEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentWindow::MeshAssetDocumentEventHandler, this));

  m_pLabelInfo = new QLabel(this);
  setCentralWidget(m_pLabelInfo);

  m_pLabelInfo->setText("<Mesh Information>");

  FinishWindowCreation();

  UpdatePreview();
}

ezCollisionMeshAssetDocumentWindow::~ezCollisionMeshAssetDocumentWindow()
{
  m_pAssetDoc->m_AssetEvents.RemoveEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentWindow::MeshAssetDocumentEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentWindow::PropertyEventHandler, this));
}

void ezCollisionMeshAssetDocumentWindow::MeshAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetDocument::AssetEvent::Type::AssetInfoChanged:
    UpdatePreview();
    break;
  }
}

void ezCollisionMeshAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((ezCollisionMeshAssetDocument*)GetDocument())->GetProperties();

  //ezStringBuilder s;
  //s.Format("Vertices: %u\nTriangles: %u\nSubMeshes: %u", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  //for (ezUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot %u: %s", m, prop->m_SlotNames[m].GetData());
  
  //m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void ezCollisionMeshAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  //if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}




