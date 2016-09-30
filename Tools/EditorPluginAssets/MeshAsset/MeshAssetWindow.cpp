#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/MeshAsset/MeshViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

ezMeshAssetDocumentWindow::ezMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument) : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MeshAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MeshAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtMeshViewWidget(nullptr, this, &m_ViewConfig);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, nullptr/*"MeshAssetViewToolBar"*/);
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("MeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  GetMeshDocument()->m_AssetEvents.AddEventHandler(ezMakeDelegate(&ezMeshAssetDocumentWindow::MeshAssetDocumentEventHandler, this));

  m_pLabelInfo = new QLabel(this);
  m_pLabelInfo->setText("<Mesh Information>");
  pContainer->GetLayout()->addWidget(m_pLabelInfo, 0);

  FinishWindowCreation();

  UpdatePreview();
}

ezMeshAssetDocumentWindow::~ezMeshAssetDocumentWindow()
{
  GetMeshDocument()->m_AssetEvents.RemoveEventHandler(ezMakeDelegate(&ezMeshAssetDocumentWindow::MeshAssetDocumentEventHandler, this));
}


ezMeshAssetDocument* ezMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<ezMeshAssetDocument*>(GetDocument());
}


void ezMeshAssetDocumentWindow::UpdatePreview()
{

}

void ezMeshAssetDocumentWindow::MeshAssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetDocument::AssetEvent::Type::AssetInfoChanged:
    UpdatePreview();
    break;
  }
}

void ezMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = false;
    msg.m_fSimulationSpeed = 1.0f;
    msg.m_fGizmoScale = ezPreferences::QueryPreferences<ezEditorPreferencesUser>()->m_fGizmoScale;
    msg.m_bRenderOverlay = false;
    msg.m_bRenderShapeIcons = false;
    msg.m_bRenderSelectionBoxes = false;
    msg.m_bAddAmbientLight = true; // not implemented yet
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  //auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  {
    ezSyncWithProcessMsgToEngine sm;
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&sm);

    ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezSyncWithProcessMsgToEditor>(), ezTime::Seconds(2.0));
  }
}

void ezMeshAssetDocumentWindow::InternalRedraw()
{
  ezQtEngineDocumentWindow::InternalRedraw();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}



