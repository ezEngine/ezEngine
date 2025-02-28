#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

ezQtBlackboardTemplateAssetDocumentWindow::ezQtBlackboardTemplateAssetDocumentWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "BlackboardTemplateAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "BlackboardTemplateAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("BlackboardTemplateAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("BlackboardTemplateAssetDockWidget");
    pPropertyPanel->setWindowTitle("BlackboardTemplate Properties");
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


    m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

ezQtBlackboardTemplateAssetDocumentWindow::~ezQtBlackboardTemplateAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler, this));

  RestoreResource();
}

void ezQtBlackboardTemplateAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto pDocument = static_cast<ezBlackboardTemplateAssetDocument*>(GetDocument());

  ezResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "BlackboardTemplate";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezBlackboardTemplate");
  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDocument->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (pDocument->WriteAsset(memoryWriter, ezAssetCurator::GetSingleton()->GetActiveAssetProfile()).Succeeded())
  {
    msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void ezQtBlackboardTemplateAssetDocumentWindow::RestoreResource()
{
  ezRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "BlackboardTemplate";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtBlackboardTemplateAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezQtBlackboardTemplateAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectRemoved)
  {
    UpdatePreview();
  }
}
