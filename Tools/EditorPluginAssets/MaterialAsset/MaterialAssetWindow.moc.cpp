#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>

ezMaterialAssetDocumentWindow::ezMaterialAssetDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  m_DelegatePropertyEvents = ezMakeDelegate(&ezMaterialAssetDocumentWindow::PropertyEventHandler, this);
  GetDocument()->GetObjectTree()->m_PropertyEvents.AddEventHandler(m_DelegatePropertyEvents);

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pImageWidget = new QtImageWidget(this);
  setCentralWidget(m_pImageWidget);

  UpdatePreview();
}

ezMaterialAssetDocumentWindow::~ezMaterialAssetDocumentWindow()
{
  GetDocument()->GetObjectTree()->m_PropertyEvents.RemoveEventHandler(m_DelegatePropertyEvents);
}

void ezMaterialAssetDocumentWindow::UpdatePreview()
{
  ezMaterialAssetObject* pObject = (ezMaterialAssetObject*)GetDocument()->GetObjectTree()->GetRootObject()->GetChildren()[0];

  // TODO
}

void ezMaterialAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  //if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}




