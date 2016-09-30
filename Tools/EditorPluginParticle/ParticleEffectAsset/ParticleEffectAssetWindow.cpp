#include <PCH.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleViewWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>

ezParticleEffectAssetDocumentWindow::ezParticleEffectAssetDocumentWindow(ezAssetDocument* pDocument) 
  : ezQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::StructureEventHandler, this));


  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ParticleEffectAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ParticleEffectAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ParticleEffectAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Property Grid
  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("ParticleEffectAssetDockWidget");
    pPropertyPanel->setWindowTitle("Particle Effect Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtParticleViewWidget(nullptr, this, &m_ViewConfig);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  m_pAssetDoc = static_cast<ezParticleEffectAssetDocument*>(pDocument);
  m_pAssetDoc->m_AssetEvents.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::AssetDocumentEventHandler, this));

  FinishWindowCreation();

  UpdatePreview();

  GetParticleDocument()->m_Events.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

ezParticleEffectAssetDocumentWindow::~ezParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::PropertyEventHandler, this));

  m_pAssetDoc->m_AssetEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentWindow::AssetDocumentEventHandler, this));
}


ezParticleEffectAssetDocument* ezParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<ezParticleEffectAssetDocument*>(GetDocument());
}

void ezParticleEffectAssetDocumentWindow::AssetDocumentEventHandler(const ezAssetDocument::AssetEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetDocument::AssetEvent::Type::AssetInfoChanged:
    UpdatePreview();
    break;
  }
}

void ezParticleEffectAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezEditorEngineResourceUpdateMsg msg;
  msg.m_sResourceType = "Particle";

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezParticleEffect");
  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetParticleDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter);
  // Write Asset Data
  GetParticleDocument()->WriteParticleEffectAsset(memoryWriter, "PC");
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize());

  GetEditorEngineConnection()->SendMessage(&msg);
}

void ezParticleEffectAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezParticleEffectAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    UpdatePreview();
    break;
  }
}


void ezParticleEffectAssetDocumentWindow::ParticleEventHandler(const ezParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
  case ezParticleEffectAssetEvent::RestartEffect:
    {
      ezEditorEngineRestartSimulationMsg msg;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

  case ezParticleEffectAssetEvent::AutoRestartChanged:
    {
      ezEditorEngineLoopAnimationMsg msg;
      msg.m_bLoop = GetParticleDocument()->GetAutoRestart();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

  default:
    break;
  }
}

void ezParticleEffectAssetDocumentWindow::InternalRedraw()
{
  ezQtEngineDocumentWindow::InternalRedraw();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}


void ezParticleEffectAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = true;
    msg.m_fSimulationSpeed = GetParticleDocument()->GetSimulationSpeed();
    msg.m_fGizmoScale = 0.0f;
    msg.m_bRenderOverlay = false;
    msg.m_bRenderShapeIcons = false;
    msg.m_bRenderSelectionBoxes = false;
    msg.m_bAddAmbientLight = true; // not implemented yet
    GetEditorEngineConnection()->SendMessage(&msg);
  }

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

void ezParticleEffectAssetDocumentWindow::RestoreResource()
{
  ezEditorEngineRestoreResourceMsg msg;
  GetEditorEngineConnection()->SendMessage(&msg);
}





