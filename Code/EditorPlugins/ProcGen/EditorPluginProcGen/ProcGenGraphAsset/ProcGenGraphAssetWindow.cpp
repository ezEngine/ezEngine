#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

ezProcGenGraphAssetDocumentWindow::ezProcGenGraphAssetDocumentWindow(ezProcGenGraphAssetDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ProcGenAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ProcGenAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ProcGenAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Central Widget
  {
    m_pScene = new ezQtProcGenScene(this);
    m_pScene->InitScene(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));

    m_pView = new ezQtNodeView(this);
    m_pView->SetScene(m_pScene);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("ProcGenGraphView");
    pCentral->setWindowTitle("Graph");
    pCentral->setWidget(m_pView);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("ProcGenAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
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
  }

  UpdatePreview();

  FinishWindowCreation();
}

ezProcGenGraphAssetDocumentWindow::~ezProcGenGraphAssetDocumentWindow()
{
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

ezProcGenGraphAssetDocument* ezProcGenGraphAssetDocumentWindow::GetProcGenGraphDocument()
{
  return static_cast<ezProcGenGraphAssetDocument*>(GetDocument());
}

void ezProcGenGraphAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezProcGenGraph");
  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetProcGenGraphDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (GetProcGenGraphDocument()->WriteAsset(memoryWriter, ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), true).Succeeded())
  {
    msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void ezProcGenGraphAssetDocumentWindow::RestoreResource()
{
  ezRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezProcGenGraphAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  // this event is only needed for changes to the DebugPin
  if (e.m_sProperty == "DebugPin")
  {
    UpdatePreview();
  }
}

void ezProcGenGraphAssetDocumentWindow::TransationEventHandler(const ezCommandHistoryEvent& e)
{
  if (e.m_Type == ezCommandHistoryEvent::Type::TransactionEnded || e.m_Type == ezCommandHistoryEvent::Type::UndoEnded || e.m_Type == ezCommandHistoryEvent::Type::RedoEnded)
  {
    UpdatePreview();
  }
}
