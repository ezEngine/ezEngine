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
#include <Foundation/Image/ImageConversion.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

ezQtMeshAssetDocumentWindow::ezQtMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument) : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
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
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "MeshAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("MeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  QueryObjectBBox();
}

ezMeshAssetDocument* ezQtMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<ezMeshAssetDocument*>(GetDocument());
}

void ezQtMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_fGizmoScale = ezPreferences::QueryPreferences<ezEditorPreferencesUser>()->m_fGizmoScale;
    msg.m_bAddAmbientLight = true; // not implemented yet
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtMeshAssetDocumentWindow::QueryObjectBBox()
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtMeshAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtMeshAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_iPurpose == 0 && pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid() && pMessage->m_vHalfExtents.x >= 0 && pMessage->m_vHalfExtents.y >= 0 && pMessage->m_vHalfExtents.z >= 0)
    {
      m_pViewWidget->GetOrbitCamera()->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents * 2.0f, ezVec3(5, -1, 2) * pMessage->m_vHalfExtents.GetLength() * 0.3f);
    }
    else
    {
      // try again
      QueryObjectBBox();
    }

    return;
  }

  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}


