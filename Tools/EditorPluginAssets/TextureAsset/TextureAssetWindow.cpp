#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>

ezTextureAssetDocumentWindow::ezTextureAssetDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_DelegatePropertyEvents = ezMakeDelegate(&ezTextureAssetDocumentWindow::PropertyEventHandler, this);
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(m_DelegatePropertyEvents);

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "TextureAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pImageWidget = new QtImageWidget(this);
  setCentralWidget(m_pImageWidget);

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  }

  UpdatePreview();
}

ezTextureAssetDocumentWindow::~ezTextureAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_DelegatePropertyEvents);
}

void ezTextureAssetDocumentWindow::UpdatePreview()
{
  ezTextureAssetObject* pObject = (ezTextureAssetObject*)GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  if (pObject->m_MemberProperties.GetImage().GetDataSize() == 0)
    return;

  ezImage Target;
  if (ezImageConversionBase::Convert(pObject->m_MemberProperties.GetImage(), Target, ezImageFormat::B8G8R8A8_UNORM).Failed())
    return;

  /// \todo Fix format once ezImage is fixed
  QImage img(Target.GetPixelPointer<ezUInt8>(), Target.GetWidth(), Target.GetHeight(), QImage::Format_RGBA8888);

  m_pImageWidget->SetImage(QPixmap::fromImage(img));
}

void ezTextureAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  if (e.m_sPropertyPath == "Texture File")
  {
    UpdatePreview();
  }
}




