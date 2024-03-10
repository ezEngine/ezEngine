#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtScene2DocumentWindow::ezQtScene2DocumentWindow(ezScene2Document* pDocument)
  : ezQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](ezQtEngineDocumentWindow* pWindow, ezEngineViewConfig* pConfig) -> ezQtEngineViewWidget*
  {
    ezQtSceneViewWidget* pWidget = new ezQtSceneViewWidget(nullptr, static_cast<ezQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new ezQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate([this](ezGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<ezGameObjectDocument*>(GetDocument()), this, this); });

  setCentralWidget(m_pQuadViewWidget);

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  SetTargetFramerate(pPreferences->GetMaxFramerate());

  {
    // Menu Bar
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2MenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2ToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  {
    // Panels
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezQtDocumentPanel* pPanelTree = new ezQtScenegraphPanel(this, pDocument);
    pPanelTree->show();

    ezQtLayerPanel* pLayers = new ezQtLayerPanel(this, pDocument);
    pLayers->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    EZ_VERIFY(connect(pPropertyGrid, &ezQtPropertyGridWidget::ExtendContextMenu, this, &ezQtScene2DocumentWindow::ExtendPropertyGridContextMenu), "");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pLayers);
  }
  FinishWindowCreation();
}

ezQtScene2DocumentWindow::~ezQtScene2DocumentWindow() = default;

bool ezQtScene2DocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  ezScene2Document* pDoc = static_cast<ezScene2Document*>(GetDocument());
  if (pDoc && pDoc->IsAnyLayerModified())
  {
    QMessageBox::StandardButton res = ezQtUiServices::MessageBoxQuestion("Save scene and all layers before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      ezStatus err = SaveAllLayers();

      if (err.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

ezStatus ezQtScene2DocumentWindow::SaveAllLayers()
{
  ezScene2Document* pDoc = static_cast<ezScene2Document*>(GetDocument());

  ezHybridArray<ezSceneDocument*, 16> layers;
  pDoc->GetLoadedLayers(layers);

  for (auto pLayer : layers)
  {
    ezStatus res = pLayer->SaveDocument();

    if (res.Failed())
    {
      return res;
    }
  }

  return ezStatus(EZ_SUCCESS);
}
