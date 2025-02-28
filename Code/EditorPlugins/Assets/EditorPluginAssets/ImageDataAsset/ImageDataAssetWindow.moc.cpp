#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
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

  // Central Widget
  {
    m_pImageWidget = new ezQtImageWidget(this);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("ImageDataView");
    pCentral->setWindowTitle("Image");
    pCentral->setWidget(m_pImageWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ImageDataProperties");
    pPropertyPanel->setWindowTitle("Image Properties");

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

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();

  pDocument->Events().AddEventHandler(ezMakeDelegate(&ezQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler, this), m_EventUnsubscriper);
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
