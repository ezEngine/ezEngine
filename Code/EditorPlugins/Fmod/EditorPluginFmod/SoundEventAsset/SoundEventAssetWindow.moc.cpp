#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

ezSoundEventAssetDocumentWindow::ezSoundEventAssetDocumentWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SoundEventAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SoundEventAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundEventAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SoundEventAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Event Properties");
    pPropertyPanel->show();

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

  m_pAssetDoc = static_cast<ezSoundEventAssetDocument*>(pDocument);

  FinishWindowCreation();

  UpdatePreview();
}

ezSoundEventAssetDocumentWindow::~ezSoundEventAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
    ezMakeDelegate(&ezSoundEventAssetDocumentWindow::PropertyEventHandler, this));
}

void ezSoundEventAssetDocumentWindow::UpdatePreview()
{
  const auto& prop = ((ezSoundEventAssetDocument*)GetDocument())->GetProperties();

  // ezStringBuilder s;
  // s.SetFormat("Vertices: {0}\nTriangles: {1}\nSubMeshes: {2}", prop->m_uiVertices, prop->m_uiTriangles, prop->m_SlotNames.GetCount());

  // for (ezUInt32 m = 0; m < prop->m_SlotNames.GetCount(); ++m)
  //  s.AppendFormat("\nSlot {0}: {1}", m, prop->m_SlotNames[m]);

  // m_pLabelInfo->setText(QString::fromUtf8(s.GetData()));
}

void ezSoundEventAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  // if (e.m_sPropertyPath == "Texture File")
  //{
  //  UpdatePreview();
  //}
}
