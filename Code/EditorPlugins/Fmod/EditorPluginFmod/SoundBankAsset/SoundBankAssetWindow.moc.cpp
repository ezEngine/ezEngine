#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorFramework/GUI/SoundPreviewWidget.moc.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankPreview.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

#include <QHeaderView>
#include <QTableWidget>

namespace
{
  // the row order of the table may differ from the event order, if the user sorts it
  constexpr int s_iEventIndexRole = Qt::UserRole + 1;
} // namespace

ezSoundBankAssetDocumentWindow::ezSoundBankAssetDocumentWindow(ezDocument* pDocument)
  : ezQtDocumentWindow(pDocument)
{
  m_pAssetDoc = static_cast<ezSoundBankAssetDocument*>(pDocument);
  m_pPreview = EZ_DEFAULT_NEW(ezSoundBankPreview);

  pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::PropertyEventHandler, this));
  m_pAssetDoc->m_SubAssetChosenEvent.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::SubAssetChosenHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SoundBankAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SoundBankAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SoundBankAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Event List and Playback Controls
  {
    m_pEventTable = new QTableWidget(this);
    m_pEventTable->setColumnCount(3);
    m_pEventTable->setHorizontalHeaderLabels({"Event", "Length", "Type"});
    m_pEventTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_pEventTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pEventTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pEventTable->verticalHeader()->hide();
    m_pEventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pEventTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pEventTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_pEventTable->setSortingEnabled(true);

    m_pPreviewWidget = new ezQtSoundPreviewWidget(this);

    connect(m_pEventTable, &QTableWidget::itemSelectionChanged, this, &ezSoundBankAssetDocumentWindow::UpdateInfoText);
    connect(m_pEventTable, &QTableWidget::cellDoubleClicked, this, [this](int, int)
      { OnPlayRequested(); });
    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::PlayRequested, this, &ezSoundBankAssetDocumentWindow::OnPlayRequested);
    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::StopRequested, this, &ezSoundBankAssetDocumentWindow::OnStopRequested);
    connect(m_pPreviewWidget, &ezQtSoundPreviewWidget::UpdateRequested, this, &ezSoundBankAssetDocumentWindow::OnUpdateRequested);

    QWidget* pContainer = new QWidget(this);
    QVBoxLayout* pLayout = new QVBoxLayout(pContainer);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_pEventTable, 1);
    pLayout->addWidget(m_pPreviewWidget, 0);

    ezQtDocumentPanel* pCentral = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pCentral->setObjectName("SoundBankAssetPreview");
    pCentral->setWindowTitle("Events");
    pCentral->setWidget(pContainer);

    m_pDockManager->setCentralWidget(pCentral);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("SoundBankAssetDockWidget");
    pPropertyPanel->setWindowTitle("Sound Bank Properties");
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

  ReloadBank();
}

ezSoundBankAssetDocumentWindow::~ezSoundBankAssetDocumentWindow()
{
  m_pAssetDoc->m_SubAssetChosenEvent.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::SubAssetChosenHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentWindow::PropertyEventHandler, this));
}

void ezSoundBankAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "SoundBankFile")
  {
    // queued, because the document properties aren't guaranteed to be in sync yet
    QMetaObject::invokeMethod(this, &ezSoundBankAssetDocumentWindow::ReloadBank, Qt::QueuedConnection);
  }
}

void ezSoundBankAssetDocumentWindow::SubAssetChosenHandler(const ezUuid& subAssetGuid)
{
  const auto& events = m_pPreview->GetEvents();

  for (int iRow = 0; iRow < m_pEventTable->rowCount(); ++iRow)
  {
    const ezUInt32 uiEventIndex = m_pEventTable->item(iRow, 0)->data(s_iEventIndexRole).toUInt();

    if (events[uiEventIndex].m_Guid == subAssetGuid)
    {
      m_pEventTable->selectRow(iRow);
      m_pEventTable->scrollToItem(m_pEventTable->item(iRow, 0));
      return;
    }
  }
}

void ezSoundBankAssetDocumentWindow::ReloadBank()
{
  m_pPreviewWidget->SetPlaybackState(false);

  const ezString& sBank = m_pAssetDoc->GetProperties()->m_sSoundBank;
  ezStringBuilder sAbsPath = sBank;

  if (sBank.IsEmpty())
  {
    m_LoadStatus = ezStatus("No sound bank file has been specified.");
  }
  else if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
  {
    m_LoadStatus = ezStatus(ezFmt("Failed to make sound bank path absolute: '{}'", sBank));
  }
  else
  {
    m_LoadStatus = m_pPreview->LoadBank(sAbsPath);
  }

  const auto& events = m_pPreview->GetEvents();

  // sorting has to be off while filling the table, otherwise rows move while they are written to
  m_pEventTable->setSortingEnabled(false);
  m_pEventTable->clearContents();
  m_pEventTable->setRowCount(events.GetCount());

  for (ezUInt32 i = 0; i < events.GetCount(); ++i)
  {
    const auto& ev = events[i];

    ezStringBuilder sLength;
    if (ev.m_Length.IsPositive())
      sLength.SetFormat("{} sec", ezArgF(ev.m_Length.GetSeconds(), 2));

    ezStringBuilder sType;
    sType.SetFormat("{}, {}", ev.m_b3D ? "3D" : "2D", ev.m_bOneShot ? "one-shot" : "persistent");

    QTableWidgetItem* pName = new QTableWidgetItem(ezMakeQString(ev.m_sName));
    pName->setData(s_iEventIndexRole, i);

    m_pEventTable->setItem(i, 0, pName);
    m_pEventTable->setItem(i, 1, new QTableWidgetItem(ezMakeQString(sLength)));
    m_pEventTable->setItem(i, 2, new QTableWidgetItem(ezMakeQString(sType)));
  }

  m_pEventTable->setSortingEnabled(true);

  if (!events.IsEmpty())
  {
    m_pEventTable->selectRow(0);
  }

  UpdateInfoText();
}

void ezSoundBankAssetDocumentWindow::UpdateInfoText()
{
  const auto& events = m_pPreview->GetEvents();
  const QList<QTableWidgetItem*> selection = m_pEventTable->selectedItems();

  m_pPreviewWidget->SetCanPlay(!selection.isEmpty());

  if (m_LoadStatus.Failed())
  {
    m_pPreviewWidget->SetInfoText(m_LoadStatus.GetMessageString());
    return;
  }

  if (events.IsEmpty())
  {
    m_pPreviewWidget->SetInfoText("This sound bank contains no events.");
    return;
  }

  ezStringBuilder sInfo;
  sInfo.SetFormat("{} event(s)", events.GetCount());

  if (!selection.isEmpty())
  {
    // the first selected item isn't necessarily in the first column
    const QTableWidgetItem* pName = m_pEventTable->item(selection[0]->row(), 0);
    const auto& ev = events[pName->data(s_iEventIndexRole).toUInt()];

    sInfo.AppendFormat("\nGUID: {}", ev.m_Guid);

    if (!ev.m_Length.IsPositive())
      sInfo.Append("\nThis event has no timeline, its length is unknown.");
  }

  m_pPreviewWidget->SetInfoText(sInfo);
}

void ezSoundBankAssetDocumentWindow::Play(ezUInt32 uiEventIndex)
{
  const ezStatus res = m_pPreview->Play(uiEventIndex);

  if (res.Failed())
  {
    m_pPreviewWidget->SetInfoText(res.GetMessageString());
    m_pPreviewWidget->SetPlaybackState(false);
    return;
  }

  m_PlayingLength = m_pPreview->GetEvents()[uiEventIndex].m_Length;
  OnUpdateRequested();
}

void ezSoundBankAssetDocumentWindow::OnPlayRequested()
{
  const QList<QTableWidgetItem*> selection = m_pEventTable->selectedItems();

  if (!selection.isEmpty())
  {
    Play(m_pEventTable->item(selection[0]->row(), 0)->data(s_iEventIndexRole).toUInt());
  }
}

void ezSoundBankAssetDocumentWindow::OnStopRequested()
{
  m_pPreview->Stop();
  m_pPreviewWidget->SetPlaybackState(false);
}

void ezSoundBankAssetDocumentWindow::OnUpdateRequested()
{
  m_pPreview->Update();

  if (m_pPreview->IsPlaying())
  {
    m_pPreviewWidget->SetPlaybackState(true, m_pPreview->GetPosition(), m_PlayingLength);
  }
  else
  {
    OnStopRequested();
  }
}
