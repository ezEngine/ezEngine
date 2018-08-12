#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <SharedPluginAssets/Common/Messages.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <VisualShader/VisualShaderScene.moc.h>


ezInt32 ezQtMaterialAssetDocumentWindow::s_iNodeConfigWatchers = 0;
ezDirectoryWatcher* ezQtMaterialAssetDocumentWindow::s_pNodeConfigWatcher = nullptr;


ezQtMaterialAssetDocumentWindow::ezQtMaterialAssetDocumentWindow(ezMaterialAssetDocument* pDocument)
    : ezQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::SelectionEventHandler, this));

  pDocument->m_VisualShaderEvents.AddEventHandler(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MaterialAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MaterialAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MaterialAssetWindowToolBar");
    addToolBar(pToolBar);
  }


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90, 0.01f, 100.0f);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0), ezVec3(0.0f), ezVec3(-0.2, 0, 0));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(nullptr, m_pViewWidget, "MaterialAssetViewToolBar");

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

    QSplitter* pSplitter = new QSplitter(Qt::Orientation::Horizontal, m_pVsePanel);

    m_pScene = new ezQtVisualShaderScene(this);
    m_pScene->SetDocumentNodeManager(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));
    m_pNodeView = new ezQtNodeView(m_pVsePanel);
    m_pNodeView->SetScene(m_pScene);
    pSplitter->addWidget(m_pNodeView);

    QWidget* pRightGroup = new QWidget(m_pVsePanel);
    pRightGroup->setLayout(new QVBoxLayout());

    QWidget* pButtonGroup = new QWidget(m_pVsePanel);
    pButtonGroup->setLayout(new QHBoxLayout());

    m_pOutputLine = new QTextEdit(m_pVsePanel);
    m_pOutputLine->setText("Transform the material asset to compile the Visual Shader.");
    m_pOutputLine->setReadOnly(true);

    m_pOpenShaderButton = new QPushButton(m_pVsePanel);
    m_pOpenShaderButton->setText("Open Shader File");
    connect(m_pOpenShaderButton, &QPushButton::clicked, this, &ezQtMaterialAssetDocumentWindow::OnOpenShaderClicked);

    pButtonGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pButtonGroup->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    pButtonGroup->layout()->addWidget(m_pOpenShaderButton);

    pRightGroup->layout()->setContentsMargins(0, 0, 0, 0);
    pRightGroup->layout()->addWidget(m_pOutputLine);
    pRightGroup->layout()->addWidget(pButtonGroup);

    pSplitter->addWidget(pRightGroup);

    pSplitter->setStretchFactor(0, 10);
    pSplitter->setStretchFactor(1, 1);

    m_bVisualShaderEnabled = false;
    m_pVsePanel->setWidget(pSplitter);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pVsePanel);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  UpdatePreview();

  UpdateNodeEditorVisibility();

  FinishWindowCreation();
}

ezQtMaterialAssetDocumentWindow::~ezQtMaterialAssetDocumentWindow()
{
  GetMaterialDocument()->m_VisualShaderEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler, this));

  RestoreResource();

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
      ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::PropertyEventHandler, this));

  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() ==
                       ezMaterialShaderMode::Custom;

  if (bCustom)
  {
    SetupDirectoryWatcher(false);
  }
}

void ezQtMaterialAssetDocumentWindow::SetupDirectoryWatcher(bool needIt)
{
  if (needIt)
  {
    ++s_iNodeConfigWatchers;

    if (s_pNodeConfigWatcher == nullptr)
    {
      s_pNodeConfigWatcher = EZ_DEFAULT_NEW(ezDirectoryWatcher);

      ezStringBuilder sSearchDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
      sSearchDir.AppendPath("VisualShader");

      if (s_pNodeConfigWatcher->OpenDirectory(sSearchDir, ezDirectoryWatcher::Watch::Writes).Failed())
        ezLog::Warning("Could not register a file system watcher for changes to '{0}'", sSearchDir);
    }
  }
  else
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
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  if (s_pNodeConfigWatcher)
  {
    s_pNodeConfigWatcher->EnumerateChanges(ezMakeDelegate(&ezQtMaterialAssetDocumentWindow::OnVseConfigChanged, this));
  }
  ezQtEngineDocumentWindow::InternalRedraw();
}


void ezQtMaterialAssetDocumentWindow::OnOpenShaderClicked(bool)
{
  ezAssetDocumentManager* pManager = (ezAssetDocumentManager*)GetMaterialDocument()->GetDocumentManager();

  ezString sAutoGenShader =
      pManager->GetAbsoluteOutputFileName(GetMaterialDocument()->GetDocumentPath(), ezMaterialAssetDocumentManager::s_szShaderOutputTag);

  if (ezOSFile::ExistsFile(sAutoGenShader))
  {
    ezQtUiServices::OpenFileInDefaultProgram(sAutoGenShader);
  }
  else
  {
    ezStringBuilder msg;
    msg.Format("The auto generated file does not exist (yet).\nThe supposed location is '{0}'", sAutoGenShader);

    ezQtUiServices::GetSingleton()->MessageBoxInformation(msg);
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
  GetMaterialDocument()->WriteMaterialAsset(memoryWriter, "PC", false);
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
    QTimer::singleShot(1, [this]() { GetDocument()->GetSelectionManager()->SetSelection(GetMaterialDocument()->GetPropertyObject()); });
  }
}

void ezQtMaterialAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtMaterialAssetDocumentWindow::RestoreResource()
{
  ezEditorEngineRestoreResourceMsg msg;
  GetEditorEngineConnection()->SendMessage(&msg);
}

void ezQtMaterialAssetDocumentWindow::UpdateNodeEditorVisibility()
{
  const bool bCustom = GetMaterialDocument()->GetPropertyObject()->GetTypeAccessor().GetValue("ShaderMode").ConvertTo<ezInt64>() ==
                       ezMaterialShaderMode::Custom;

  // when this is called during construction, it seems to be overridden again (probably by the dock widget code or the splitter)
  // by delaying it a bit, we have the last word
  QTimer::singleShot(10, this, [this, bCustom]() { m_pVsePanel->setVisible(bCustom); });

  if (m_bVisualShaderEnabled != bCustom)
  {
    m_bVisualShaderEnabled = bCustom;

    SetupDirectoryWatcher(bCustom);
  }
}

void ezQtMaterialAssetDocumentWindow::OnVseConfigChanged(const char* filename, ezDirectoryWatcherAction action)
{
  if (!ezPathUtils::HasExtension(filename, "DDL"))
    return;

  // lalala ... this is to allow writes to the file to 'hopefully' finish before we try to read it
  ezThreadUtils::Sleep(ezTime::Milliseconds(100));

  ezVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData(filename);

  // TODO: We write an invalid hash in the file, should maybe compute the correct one on the fly
  // but that would involve the asset curator which would also save / transform everything which is
  // not what we want.
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(0, GetMaterialDocument()->GetAssetTypeVersion());
  GetMaterialDocument()->RecreateVisualShaderFile("", AssetHeader);
}

void ezQtMaterialAssetDocumentWindow::VisualShaderEventHandler(const ezMaterialVisualShaderEvent& e)
{
  ezStringBuilder text;

  if (e.m_Type == ezMaterialVisualShaderEvent::VisualShaderNotUsed)
  {
    text = "<span style=\"color:#bbbb00;\">Visual Shader is not used by the material.</span><br><br>Change the ShaderMode in the asset "
           "properties to enable Visual Shader mode.";
  }
  else
  {
    if (e.m_Type == ezMaterialVisualShaderEvent::TransformSucceeded)
      text = "<span style=\"color:#00ff00;\">Visual Shader was transformed successfully.</span><br><br>";
    else
      text = "<span style=\"color:#ff8800;\">Visual Shader is invalid:</span><br><br>";

    ezStringBuilder err = e.m_sTransformError;

    ezHybridArray<ezStringView, 16> lines;
    err.Split(false, lines, "\n");

    for (const ezStringView& line : lines)
    {
      if (line.StartsWith("Error:"))
        text.AppendFormat("<span style=\"color:#ff2200;\">{0}</span><br>", line);
      else if (line.StartsWith("Warning:"))
        text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", line);
      else
        text.Append(line);
    }
    UpdatePreview();
  }

  m_pOutputLine->setAcceptRichText(true);
  m_pOutputLine->setHtml(text.GetData());
}
