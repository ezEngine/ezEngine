#include <PCH.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QStatusBar>
#include <QSettings>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>
#include <QToolButton>
#include <QPainter>
#include <QBoxLayout>

ezQtCuratorControl::ezQtCuratorControl(QWidget* pParent)
  : QWidget(pParent)
  , m_bScheduled(false)
  , m_pBackgroundProcess(nullptr)
{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  pLayout->addWidget(m_pBackgroundProcess);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &ezQtCuratorControl::BackgroundProcessClicked);
  pLayout->addSpacing(200);

  UpdateBackgroundProcessState();
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtCuratorControl::AssetCuratorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezQtCuratorControl::ProjectEvents, this));
}

ezQtCuratorControl::~ezQtCuratorControl()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCuratorControl::AssetCuratorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCuratorControl::ProjectEvents, this));
}

void ezQtCuratorControl::paintEvent(QPaintEvent* e)
{
  QRect rect = contentsRect();
  QRect rectButton = m_pBackgroundProcess->geometry();
  rect.setLeft(rectButton.right());

  QPainter painter(this);
  painter.setPen(QPen(Qt::NoPen));

  ezUInt32 uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb);
  const ezUInt32 uiDone = uiNumAssets - uiNumUnknown - uiNumNeedTransform - uiNumNeedThumb;
  ezUInt32 sections[4] = { uiDone, uiNumNeedThumb, uiNumNeedTransform, uiNumUnknown };
  QColor colors[4] = {QColor(Qt::darkGreen), QColor(Qt::darkBlue), QColor(Qt::blue), QColor(Qt::gray) };

  const float fTotalCount = uiNumAssets;
  const ezInt32 iTargetWidth = rect.width();
  ezInt32 iCurrentCount = 0;
  for (ezInt32 i = 0; i < 4; ++i)
  {
    ezInt32 iStartX = ezInt32((iCurrentCount / fTotalCount) * iTargetWidth);
    iCurrentCount += sections[i];
    ezInt32 iEndX = ezInt32((iCurrentCount / fTotalCount) * iTargetWidth);

    if (sections[i])
    {
      QRect area = rect;
      area.setLeft(rect.left() + iStartX);
      area.setRight(rect.left() + iEndX);
      painter.setBrush(QBrush(colors[i]));
      painter.drawRect(area);
    }
  }

  ezStringBuilder s;
  s.Format("[Un: %u, Tr: %i, Th: %u]", uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb);

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void ezQtCuratorControl::UpdateBackgroundProcessState()
{
  bool bRunning = ezAssetCurator::GetSingleton()->IsProcessTaskRunning();
  if (bRunning)
    m_pBackgroundProcess->setIcon(ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause16.png"));
  else
    m_pBackgroundProcess->setIcon(ezUIServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart16.png"));
  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(bRunning);
}

void ezQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  if (checked)
    ezAssetCurator::GetSingleton()->RestartProcessTask();
  else
    ezAssetCurator::GetSingleton()->ShutdownProcessTask();
}

void ezQtCuratorControl::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  ezUInt32 uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb);

  ezStringBuilder s;

  if (uiNumAssets > 0)
  {
    const ezUInt32 uiDone = uiNumAssets - uiNumUnknown - uiNumNeedTransform - uiNumNeedThumb;
    s.Format("Unknown: %u\nTransform Needed: %i\nThumbnail Needed: %u", uiNumUnknown, uiNumNeedTransform, uiNumNeedThumb);
    setToolTip(s.GetData());
  }
  else
  {
    setToolTip("");
  }
  update();
}

void ezQtCuratorControl::ScheduleUpdateTransformStats()
{
  if (m_bScheduled)
    return;

  m_bScheduled = true;

  QTimer::singleShot(200, this, SLOT(SlotUpdateTransformStats()));
}

void ezQtCuratorControl::AssetCuratorEvents(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
  case ezAssetCuratorEvent::Type::AssetUpdated:
    ScheduleUpdateTransformStats();
    break;
  case ezAssetCuratorEvent::Type::ProcessTaskStateChanged:
    {
      UpdateBackgroundProcessState();
    }
    break;
  default:
    break;
  }
}

void ezQtCuratorControl::ProjectEvents(const ezToolsProjectEvent& e)
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
