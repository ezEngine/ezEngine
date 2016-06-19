#include <PCH.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QStatusBar>
#include <QSettings>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>

EZ_IMPLEMENT_SINGLETON(ezQtAssetBrowserPanel);

ezQtAssetBrowserPanel::ezQtAssetBrowserPanel()
  : ezQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  m_bScheduled = false;

  m_pStatusBar = new QStatusBar(this);
  m_pStatusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  //m_pStatusText = new QLabel(this);
  m_pProgress = new QProgressBar(this);
  m_pProgress->setMinimum(0);
  m_pProgress->setMaximum(0); // busy
  m_pProgress->setMaximumWidth(250);

  //m_pStatusBar->addPermanentWidget(m_pStatusText);
  m_pStatusBar->addPermanentWidget(m_pProgress);

  dockWidgetContents->layout()->addWidget(m_pStatusBar);

  setWindowIcon(ezUIServices::GetCachedIconResource(":/EditorFramework/Icons/Asset16.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.AssetBrowser")));

  EZ_VERIFY(connect(AssetBrowserWidget, SIGNAL(ItemChosen(QString, QString, QString)), this, SLOT(SlotAssetChosen(QString, QString, QString))) != nullptr, "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserPanel::AssetCuratorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserPanel::ProjectEvents, this));
}

ezQtAssetBrowserPanel::~ezQtAssetBrowserPanel()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserPanel::AssetCuratorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserPanel::ProjectEvents, this));

  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void ezQtAssetBrowserPanel::SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  ezQtEditorApp::GetSingleton()->OpenDocument(sAssetPathAbsolute.toUtf8().data());
}

void ezQtAssetBrowserPanel::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  ezUInt32 uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb);

  ezStringBuilder s;

  if (uiNumAssets > 0)
  {
    if (uiNumUnknown > 0)
    {
      const ezUInt32 uiScanned = uiNumAssets - uiNumUnknown;

      s.Format("Scanning %u... (%%p%%)", uiNumUnknown);

      m_pProgress->setFormat(s.GetData());
      m_pProgress->setMaximum(uiNumAssets);
      m_pProgress->setValue(uiScanned);
    }
    else
    {
      m_pProgress->setFormat("%p%");
      m_pProgress->setMaximum(uiNumAssets);
      m_pProgress->setValue(uiNumAssets);
    }
  }
  else
  {
    m_pProgress->setMaximum(0); // busy
    return;
  }

  //s.Format("Scanning: %u, Need Transform: %u", uiNumUnknown, uiNumNeedTransform);
  //m_pStatusText->setText(s.GetData());
}

void ezQtAssetBrowserPanel::ScheduleUpdateTransformStats()
{
  if (m_bScheduled)
    return;

  m_bScheduled = true;

  QTimer::singleShot(500, this, SLOT(SlotUpdateTransformStats()));
}

void ezQtAssetBrowserPanel::AssetCuratorEvents(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetCuratorEvent::Type::AssetUpdated:
    ScheduleUpdateTransformStats();
    break;

  default:
    break;
  }
}

void ezQtAssetBrowserPanel::ProjectEvents(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezToolsProjectEvent::Type::ProjectClosing:
  case ezToolsProjectEvent::Type::ProjectClosed:
  case ezToolsProjectEvent::Type::ProjectOpened:
    ScheduleUpdateTransformStats();
    break;

  default:
    break;
  }
}


