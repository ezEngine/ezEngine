#include <PCH.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>

ezSurfaceAssetDocumentWindow::ezSurfaceAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SurfaceAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SurfaceAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SurfaceAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("SurfaceAssetDockWidget");
    pPropertyPanel->setWindowTitle("Surface Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

ezSurfaceAssetDocumentWindow::~ezSurfaceAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentWindow::PropertyEventHandler, this));
}

void ezSurfaceAssetDocumentWindow::UpdatePreview()
{

  // TODO
}

void ezSurfaceAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{

}




