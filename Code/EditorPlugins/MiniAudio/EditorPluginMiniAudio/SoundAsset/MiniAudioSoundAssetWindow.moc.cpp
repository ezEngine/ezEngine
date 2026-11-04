#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/GUI/SoundPreviewWidget.moc.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAssetWindow.moc.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundPreview.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

ezMiniAudioSoundAssetDocumentWindow::ezMiniAudioSoundAssetDocumentWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  m_pAssetDoc = static_cast<ezMiniAudioSoundAssetDocument*>(pDocument);
  m_pPreview = EZ_DEFAULT_NEW(ezMiniAudioSoundPreview);
  m_Rng.InitializeFromCurrentTime();

  pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezMiniAudioSoundAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MiniAudioSoundAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MiniAudioSoundAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MiniAudioSoundAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Playback Controls
  {
    m_pPreviewWidget = new ezQtSoundPreviewWidget(this);

    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::PlayRequested, this, &ezMiniAudioSoundAssetDocumentWindow::OnPlayRequested);
    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::StopRequested, this, &ezMiniAudioSoundAssetDocumentWindow::OnStopRequested);
    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::UpdateRequested, this, &ezMiniAudioSoundAssetDocumentWindow::OnUpdateRequested);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("MiniAudioSoundAssetPreview");
    pCentral->setWindowTitle("Preview");
    pCentral->setWidget(m_pPreviewWidget);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("MiniAudioSoundAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Properties");
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

  FinishWindowCreation();

  UpdateFileInfo();
}

ezMiniAudioSoundAssetDocumentWindow::~ezMiniAudioSoundAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezMiniAudioSoundAssetDocumentWindow::PropertyEventHandler, this));
}

void ezMiniAudioSoundAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "Files")
  {
    // queued, because the document properties aren't guaranteed to be in sync yet and because a single edit can fire multiple events
    QMetaObject::invokeMethod(this, &ezMiniAudioSoundAssetDocumentWindow::UpdateFileInfo, Qt::QueuedConnection);
  }
}

void ezMiniAudioSoundAssetDocumentWindow::UpdateFileInfo()
{
  ezStringBuilder sInfo;
  m_PlayableFiles.Clear();

  for (const ezString& sFile : m_pAssetDoc->GetProperties()->m_SoundFiles)
  {
    if (sFile.IsEmpty())
      continue;

    if (!sInfo.IsEmpty())
      sInfo.Append("\n");

    ezStringBuilder sAbsPath = sFile;
    ezMiniAudioSoundPreview::FileInfo info;

    if (ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath) && ezMiniAudioSoundPreview::QueryFileInfo(sAbsPath, info).Succeeded())
    {
      PlayableFile& file = m_PlayableFiles.ExpandAndGetRef();
      file.m_sAbsolutePath = sAbsPath;
      file.m_Length = info.m_Length;

      sInfo.AppendFormat("{}: {} sec, {} Hz, {} channel(s)", sFile, ezArgF(info.m_Length.GetSeconds(), 2), info.m_uiSampleRate, info.m_uiChannels);
    }
    else
    {
      sInfo.AppendFormat("{}: can't be read", sFile);
    }
  }

  m_pPreviewWidget->SetInfoText(sInfo);
  m_pPreviewWidget->SetCanPlay(!m_PlayableFiles.IsEmpty());
}

void ezMiniAudioSoundAssetDocumentWindow::OnPlayRequested()
{
  const ezMiniAudioSoundAssetProperties* pProp = m_pAssetDoc->GetProperties();

  if (m_PlayableFiles.IsEmpty())
    return;

  // same selection logic as ezMiniAudioSoundResource, so that the preview sounds like the game
  const PlayableFile& file = m_PlayableFiles[m_Rng.UInt32Index(m_PlayableFiles.GetCount())];

  auto GetRandom = [this](float fValue0, float fValue1)
  {
    const float fMin = ezMath::Min(fValue0, fValue1);
    const float fMax = ezMath::Max(fValue0, fValue1);
    return fMin < fMax ? m_Rng.FloatMinMax(fMin, fMax) : fMin;
  };

  const float fVolume = GetRandom(pProp->m_fMinVolume, pProp->m_fMaxVolume);
  const float fPitch = GetRandom(pProp->m_fMinPitch, pProp->m_fMaxPitch);

  if (m_pPreview->Play(file.m_sAbsolutePath, file.m_Length, pProp->m_bLoop, fVolume, fPitch).Failed())
  {
    ezStringBuilder sInfo;
    sInfo.SetFormat("Failed to play '{}', see the log for details.", file.m_sAbsolutePath);
    m_pPreviewWidget->SetInfoText(sInfo);
    m_pPreviewWidget->SetPlaybackState(false);
    return;
  }

  OnUpdateRequested();
}

void ezMiniAudioSoundAssetDocumentWindow::OnStopRequested()
{
  m_pPreview->Stop();
  m_pPreviewWidget->SetPlaybackState(false);
}

void ezMiniAudioSoundAssetDocumentWindow::OnUpdateRequested()
{
  if (m_pPreview->IsPlaying())
  {
    m_pPreviewWidget->SetPlaybackState(true, m_pPreview->GetPosition(), m_pPreview->GetLength());
  }
  else
  {
    OnStopRequested();
  }
}
