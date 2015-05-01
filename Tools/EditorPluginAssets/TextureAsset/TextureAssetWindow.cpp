#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>

ezTextureAssetDocumentWindow::ezTextureAssetDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  GetDocument()->GetObjectTree()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::PropertyEventHandler, this));

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

  UpdatePreview();
}

ezTextureAssetDocumentWindow::~ezTextureAssetDocumentWindow()
{
  GetDocument()->GetObjectTree()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::PropertyEventHandler, this));
}

void ezTextureAssetDocumentWindow::UpdatePreview()
{
  ezTextureAssetObject* pObject = (ezTextureAssetObject*)GetDocument()->GetObjectTree()->GetRootObject()->GetChildren()[0];

  if (pObject->m_MemberProperties.GetImage().GetDataSize() == 0)
    return;

  ezImage Target;
  if (ezImageConversionBase::Convert(pObject->m_MemberProperties.GetImage(), Target, ezImageFormat::B8G8R8A8_UNORM).Failed())
    return;

  QImage img(Target.GetPixelPointer<ezUInt8>(), Target.GetWidth(), Target.GetHeight(), QImage::Format_RGBA8888);

  m_pImageWidget->SetImage(QPixmap::fromImage(img));
}

void ezTextureAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  if (e.m_sPropertyPath == "Texture File")
  {
    UpdatePreview();
  }
}




