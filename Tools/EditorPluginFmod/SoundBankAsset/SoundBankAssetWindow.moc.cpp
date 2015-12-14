#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetObjects.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>

ezSoundBankAssetDocumentWindow::ezSoundBankAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SoundBankAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "SoundBankAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundBankAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("SoundBankAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Bank Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pAssetDoc = static_cast<ezSoundBankAssetDocument*>(pDocument);
  m_pAssetDoc->m_AssetEvents.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::SoundBankAssetDocumentEventHandler, this));

  m_pLabelInfo = new QLabel(this);
  setCentralWidget(m_pLabelInfo);

  m_pLabelInfo->setText("<Information>");

  FinishWindowCreation();

  UpdatePreview();
}

ezSoundBankAssetDocumentWindow::~ezSoundBankAssetDocumentWindow()
{
  m_pAssetDoc->m_AssetEvents.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::SoundBankAssetDocumentEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::PropertyEventHandler, this));
}

void ezSoundBankAssetDocumentWindow::SoundBankAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetDocument::AssetEvent::Type::AssetInfoChanged:
    UpdatePreview();
    break;
  }
}

void ezSoundBankAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((ezSoundBankAssetDocument*)GetDocument())->GetProperties();

  //ezStringBuilder s;
  //s.Format("Vertices: %u\nTriangles: %u\nSubMeshes: %u", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  //for (ezUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot %u: %s", m, prop->m_SlotNames[m].GetData());
  
  //m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void ezSoundBankAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  //if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}




