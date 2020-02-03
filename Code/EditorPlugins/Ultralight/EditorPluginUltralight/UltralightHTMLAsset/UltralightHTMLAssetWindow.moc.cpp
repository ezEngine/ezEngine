#include <PCH.h>

#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>

ezUltralightHTMLAssetDocumentWindow::ezUltralightHTMLAssetDocumentWindow(ezDocument* pDocument)
    : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezUltralightHTMLAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "UltralightHTMLAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "UltralightHTMLAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("UltralightHTMLAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("UltralightHTMLAssetDockWidget");
    pPropertyPanel->setWindowTitle("Ultralight HTML Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezUltralightHTMLAssetDocument*>(pDocument);

  m_pLabelInfo = new QLabel(this);
  setCentralWidget(m_pLabelInfo);

  m_pLabelInfo->setText("<Information>");

  FinishWindowCreation();

  UpdatePreview();
}

ezUltralightHTMLAssetDocumentWindow::~ezUltralightHTMLAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
      ezMakeDelegate(&ezUltralightHTMLAssetDocumentWindow::PropertyEventHandler, this));
}

void ezUltralightHTMLAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((ezUltralightHTMLAssetDocument*)GetDocument())->GetProperties();

  // ezStringBuilder s;
  // s.Format("Vertices: {0}\nTriangles: {1}\nSubMeshes: {2}", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  // for (ezUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot {0}: {1}", m, prop->m_SlotNames[m]);

  // m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void ezUltralightHTMLAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  // if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}
