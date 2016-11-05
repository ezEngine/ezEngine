#include <PCH.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>

ezSoundEventAssetDocumentWindow::ezSoundEventAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SoundEventAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SoundEventAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundEventAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("SoundEventAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Event Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezSoundEventAssetDocument*>(pDocument);

  m_pLabelInfo = new QLabel(this);
  setCentralWidget(m_pLabelInfo);

  m_pLabelInfo->setText("<Information>");

  FinishWindowCreation();

  UpdatePreview();
}

ezSoundEventAssetDocumentWindow::~ezSoundEventAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentWindow::PropertyEventHandler, this));
}

void ezSoundEventAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((ezSoundEventAssetDocument*)GetDocument())->GetProperties();

  //ezStringBuilder s;
  //s.Format("Vertices: %u\nTriangles: %u\nSubMeshes: %u", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  //for (ezUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot %u: %s", m, prop->m_SlotNames[m].GetData());

  //m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void ezSoundEventAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  //if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}




