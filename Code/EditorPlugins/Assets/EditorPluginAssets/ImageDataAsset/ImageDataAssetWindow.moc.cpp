#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// ezQtImageDataAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtImageDataAssetDocumentWindow::ezQtImageDataAssetDocumentWindow(ezImageDataAssetDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ImageDataAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ImageDataAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ImageDataAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("ImageDataAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pImageWidget = new ezQtImageWidget(this);

  setCentralWidget(m_pImageWidget);

  FinishWindowCreation();

  UpdatePreview();

  pDocument->Events().AddEventHandler(ezMakeDelegate(&ezQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler, this), m_eventUnsubscriper);
}

void ezQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler(const ezImageDataAssetEvent& e)
{
  if (e.m_Type != ezImageDataAssetEvent::Type::Transformed)
    return;

  UpdatePreview();
}

void ezQtImageDataAssetDocumentWindow::UpdatePreview()
{
  auto pImageDoc = static_cast<ezImageDataAssetDocument*>(GetDocument());

  ezStringBuilder path = pImageDoc->GetProperties()->m_sInputFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(path))
    return;

  QPixmap pixmap;
  if (!pixmap.load(path.GetData(), nullptr, Qt::AutoColor))
    return;

  m_pImageWidget->SetImage(pixmap);
}
