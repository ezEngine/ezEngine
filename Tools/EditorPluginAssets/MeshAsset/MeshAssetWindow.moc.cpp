#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>

ezMeshAssetDocumentWindow::ezMeshAssetDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezMeshAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MeshAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "MeshAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("MeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");
    pPropertyPanel->show();

    ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  UpdatePreview();
}

ezMeshAssetDocumentWindow::~ezMeshAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezMeshAssetDocumentWindow::PropertyEventHandler, this));
}

void ezMeshAssetDocumentWindow::UpdatePreview()
{
  //ezMeshAssetObject* pObject = (ezMeshAssetObject*)GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  
  // TODO
}

void ezMeshAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  //if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}




