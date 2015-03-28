#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>

ezTextureAssetDocumentWindow::ezTextureAssetDocumentWindow(ezDocumentBase* pDocument) : ezDocumentWindow(pDocument)
{
  //GetDocument()->GetObjectTree()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::DocumentTreeEventHandler, this));
  //GetDocument()->GetObjectTree()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::PropertyEventHandler, this));

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
}

ezTextureAssetDocumentWindow::~ezTextureAssetDocumentWindow()
{
  //GetDocument()->GetObjectTree()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::PropertyEventHandler, this));
  //GetDocument()->GetObjectTree()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentWindow::DocumentTreeEventHandler, this));
}

//void ezTextureAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
//{
//}
//
//void ezTextureAssetDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectTreeStructureEvent& e)
//{
//}
