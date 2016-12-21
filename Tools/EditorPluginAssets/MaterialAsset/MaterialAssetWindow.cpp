#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialViewWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <SharedPluginAssets/Common/Messages.h>
#include <VisualShader/VisualShaderScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <QSplitter>
#include <QTimer>
#include <QTextEdit>


ezInt32 ezQtMaterialAssetDocumentWindow::s_iNodeConfigWatchers = 0;
ezDirectoryWatcher* ezQtMaterialAssetDocumentWindow::s_pNodeConfigWatcher = nullptr;


ezQtMaterialAssetDocumentWindow::ezQtMaterialAssetDocumentWindow(ezMaterialAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::SelectionEventHandler, this));

  pDocument->m_VisualShaderEvents.AddEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));
  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::EngineProcessMsgHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtMaterialViewWidget(nullptr, this, &m_ViewConfig);

    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(m_pVsePanel, m_pViewWidget, "MaterialAssetViewToolBar");

    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("MaterialAssetDockWidget");
    pPropertyPanel->setWindowTitle("Material Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  // Visual Shader Editor
  {
    m_pVsePanel = new ezQtDocumentPanel(this);
    m_pVsePanel->setObjectName("VisualShaderDockWidget");
    m_pVsePanel->setWindowTitle("Visual Shader Editor");
    m_pVsePanel->show();

    QSplitter* pSplitter = new QSplitter(Qt::Orientation::Horizontal, m_pVsePanel);
    //pSplitter->setChildrenCollapsible(false);

    m_pScene = new ezQtVisualShaderScene(this);
    m_pScene->SetDocumentNodeManager(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));
    m_pNodeView = new ezQtNodeView(m_pVsePanel);
    m_pNodeView->SetScene(m_pScene);
    pSplitter->addWidget(m_pNodeView);

    m_pOutputLine = new QTextEdit(m_pVsePanel);
    m_pOutputLine->setText("Transform the material asset to compile the Visual Shader.");
    m_pOutputLine->setReadOnly(true);
    pSplitter->addWidget(m_pOutputLine);

    pSplitter->setStretchFactor(0, 10);
    pSplitter->setStretchFactor(1, 1);

    m_bVisualShaderEnabled = false;
    m_pVsePanel->setWidget(pSplitter);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pVsePanel);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  FinishWindowCreation();

  UpdatePreview();

  UpdateNodeEditorVisibility();
}

ezQtMaterialAssetDocumentWindow::~ezQtMaterialAssetDocumentWindow()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::EngineProcessMsgHandler, this));
  GetMaterialDocument()->m_VisualShaderEvents.RemoveEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  RestoreResource();

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::PropertyEventHandler, this));

  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() == ezMaterialShaderMode::Custom;

  if (bCustom)
  {
    --s_iNodeConfigWatchers;

    if (s_iNodeConfigWatchers == 0)
    {
      EZ_DEFAULT_DELETE(s_pNodeConfigWatcher);
    }
  }
}

ezMaterialAssetDocument* ezQtMaterialAssetDocumentWindow::GetMaterialDocument()
{
  return static_cast<ezMaterialAssetDocument*>(GetDocument());
}

void ezQtMaterialAssetDocumentWindow::InternalRedraw()
{
  ezQtEngineDocumentWindow::InternalRedraw();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();

  if (s_pNodeConfigWatcher)
  {
    s_pNodeConfigWatcher->EnumerateChanges(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::OnVseConfigChanged, this));
  }
}

void ezQtMaterialAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezEditorEngineResourceUpdateMsg msg;
  msg.m_sResourceType = "Material";

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetMaterialDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezMaterialBin");
  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetMaterialDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetMaterialDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter);
  // Write Asset Data
  GetMaterialDocument()->WriteMaterialAsset(memoryWriter, "PC");
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize());

  GetEditorEngineConnection()->SendMessage(&msg);
}

void ezQtMaterialAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == GetMaterialDocument()->GetPropertyObject() && e.m_sProperty == "ShaderMode")
  {
    UpdateNodeEditorVisibility();
  }

  UpdatePreview();
}


void ezQtMaterialAssetDocumentWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]()
    {
      GetDocument()->GetSelectionManager()->SetSelection(GetMaterialDocument()->GetPropertyObject());
    });

  }
}

void ezQtMaterialAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_fGizmoScale = 0;
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

void ezQtMaterialAssetDocumentWindow::RestoreResource()
{
  ezEditorEngineRestoreResourceMsg msg;
  GetEditorEngineConnection()->SendMessage(&msg);
}

void ezQtMaterialAssetDocumentWindow::UpdateNodeEditorVisibility()
{
  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() == ezMaterialShaderMode::Custom;

  if (m_bVisualShaderEnabled != bCustom)
  {
    m_bVisualShaderEnabled = bCustom;
    //m_pNodeView->setEnabled(m_bVisualShaderEnabled);

    if (bCustom)
      ++s_iNodeConfigWatchers;
    else
      --s_iNodeConfigWatchers;

    if (bCustom && s_pNodeConfigWatcher == nullptr)
    {
      s_pNodeConfigWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);

      ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
      sSearchDir.AppendPath("VisualShader");

      if (s_pNodeConfigWatcher->OpenDirectory(sSearchDir, ezDirectoryWatcher::Watch::Writes).Failed())
        ezLog::Warning("Could not register a file system watcher for changes to '{0}'", sSearchDir.GetData());
    }
  }
}

void ezQtMaterialAssetDocumentWindow::OnVseConfigChanged(const char* filename, ezDirectoryWatcherAction action)
{
  if (!ezPathUtils::HasExtension(filename, "DDL"))
    return;

  // lalala ... this is to allow writes to the file to 'hopefully' finish before we try to read it
  ezThreadUtils::Sleep(100);

  ezVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData(filename);

  GetMaterialDocument()->RecreateVisualShaderFile();
}

void ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler(const ezMaterialVisualShaderEvent& e)
{
  ezStringBuilder text;

  if (e.m_Type == ezMaterialVisualShaderEvent::VisualShaderNotUsed)
  {
    m_ShowShaderMessages.SetZero();
    text = "<span style=\"color:#bbbb00;\">Visual Shader is not used by the material.</span><br><br>Change the ShaderMode in the asset properties to enable Visual Shader mode.";
  }
  else if (e.m_Type == ezMaterialVisualShaderEvent::TransformSucceeded)
  {
    m_ShowShaderMessages = ezTime::Now();
    text = "<span style=\"color:#00ff00;\">Visual Shader was transformed successfully.</span><br><br>";
    text.Append(e.m_sTransformError.GetData());
  }
  else
  {
    m_ShowShaderMessages = ezTime::Now();
    text = "<span style=\"color:#ff0000;\">Error generating the Visual Shader code.</span><br><br>";
    text.Append(e.m_sTransformError.GetData());
  }

  m_pOutputLine->setAcceptRichText(true);
  m_pOutputLine->setHtml(text.GetData());
}

void ezQtMaterialAssetDocumentWindow::EngineProcessMsgHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
    {
      // this is a ... workaround ... for the fact that we currently cannot get compilation status for a specific shader
      // so whenever a material is transformed, we assume that the shader is also compiled soon and then we display
      // all error messages with "shader" in the text in the material status window
      if (ezTime::Now() - m_ShowShaderMessages > ezTime::Seconds(3.0f))
        return;

      if (e.m_pMsg->GetDynamicRTTI()->IsDerivedFrom<ezLogMsgToEditor>())
      {
        const ezLogMsgToEditor* pMsg = static_cast<const ezLogMsgToEditor*>(e.m_pMsg);

        const ezLogMsgType::Enum type = (ezLogMsgType::Enum)pMsg->m_iMsgType;
        const bool isErrorOrWarning = (type >= ezLogMsgType::ErrorMsg && type <= ezLogMsgType::WarningMsg);

        if ((isErrorOrWarning && pMsg->m_sText.FindSubString_NoCase("shader") != nullptr) || pMsg->m_sTag == "shader")
        {
          ezStringBuilder sOutputText = m_pOutputLine->toHtml().toUtf8().data();

          if (type >= ezLogMsgType::ErrorMsg)
            sOutputText.AppendFormat("<br><br><span style=\"color:#ff0000;\">{0}</span>", pMsg->m_sText);
          else if (type >= ezLogMsgType::SeriousWarningMsg)
            sOutputText.AppendFormat("<br><br><span style=\"color:#ffaa00;\">{0}</span>", pMsg->m_sText);
          else if (type >= ezLogMsgType::WarningMsg)
            sOutputText.AppendFormat("<br><br><span style=\"color:#ffff00;\">{0}</span>", pMsg->m_sText);
          else
            sOutputText.AppendFormat("<br><br>{0}", pMsg->m_sText);

          m_pOutputLine->setHtml(sOutputText.GetData());
        }
      }
    }
    break;

  default:
    return;
  }
}
