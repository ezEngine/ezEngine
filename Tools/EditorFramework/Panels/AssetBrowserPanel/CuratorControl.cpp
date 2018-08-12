#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <QBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QToolButton>

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
  ezAssetProcessor::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtCuratorControl::AssetProcessorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezQtCuratorControl::ProjectEvents, this));
}

ezQtCuratorControl::~ezQtCuratorControl()
{
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCuratorControl::AssetCuratorEvents, this));
  ezAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCuratorControl::AssetProcessorEvents, this));
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtCuratorControl::ProjectEvents, this));
}

void ezQtCuratorControl::paintEvent(QPaintEvent* e)
{
  QRect rect = contentsRect();
  QRect rectButton = m_pBackgroundProcess->geometry();
  rect.setLeft(rectButton.right());

  QPainter painter(this);
  painter.setPen(QPen(Qt::NoPen));

  ezUInt32 uiNumAssets;
  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);
  QColor colors[ezAssetInfo::TransformState::COUNT];
  colors[ezAssetInfo::TransformState::Unknown] = QColor(Qt::gray);
  colors[ezAssetInfo::TransformState::Updating] = QColor(Qt::gray);
  colors[ezAssetInfo::TransformState::NeedsTransform] = QColor(Qt::blue);
  colors[ezAssetInfo::TransformState::NeedsThumbnail] = QColor(Qt::darkBlue);
  colors[ezAssetInfo::TransformState::UpToDate] = QColor(Qt::darkGreen);
  colors[ezAssetInfo::TransformState::MissingDependency] = QColor(Qt::red);
  colors[ezAssetInfo::TransformState::MissingReference] = QColor(Qt::red);
  colors[ezAssetInfo::TransformState::TransformError] = QColor(Qt::red);

  const float fTotalCount = uiNumAssets;
  const ezInt32 iTargetWidth = rect.width();
  ezInt32 iCurrentCount = 0;
  for (ezInt32 i = 0; i < ezAssetInfo::TransformState::COUNT; ++i)
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
  s.Format("[Un: {0}, Tr: {1}, Th: {2}, Err: {3}]", sections[ezAssetInfo::TransformState::Unknown],
           sections[ezAssetInfo::TransformState::NeedsTransform], sections[ezAssetInfo::TransformState::NeedsThumbnail],
           sections[ezAssetInfo::TransformState::MissingDependency] + sections[ezAssetInfo::TransformState::MissingReference] +
               sections[ezAssetInfo::TransformState::TransformError]);

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void ezQtCuratorControl::UpdateBackgroundProcessState()
{
  bool bRunning = ezAssetProcessor::GetSingleton()->IsProcessTaskRunning();
  if (bRunning)
    m_pBackgroundProcess->setIcon(
        ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause16.png"));
  else
    m_pBackgroundProcess->setIcon(
        ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart16.png"));
  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(bRunning);
}

void ezQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  if (checked)
    ezAssetProcessor::GetSingleton()->RestartProcessTask();
  else
    ezAssetProcessor::GetSingleton()->ShutdownProcessTask();
}

void ezQtCuratorControl::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  ezUInt32 uiNumAssets;
  ezHybridArray<ezUInt32, ezAssetInfo::TransformState::COUNT> sections;
  ezAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  ezStringBuilder s;

  if (uiNumAssets > 0)
  {
    s.Format("Unknown: {0}\nTransform Needed: {1}\nThumbnail Needed: {2}\nMissing Dependency: {3}\nMissing Reference: {4}\nFailed "
             "Transform: {5}",
             sections[ezAssetInfo::TransformState::Unknown], sections[ezAssetInfo::TransformState::NeedsTransform],
             sections[ezAssetInfo::TransformState::NeedsThumbnail], sections[ezAssetInfo::TransformState::MissingDependency],
             sections[ezAssetInfo::TransformState::MissingReference], sections[ezAssetInfo::TransformState::TransformError]);
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
    default:
      break;
  }
}
void ezQtCuratorControl::AssetProcessorEvents(const ezAssetProcessorEvent& e)
{
  switch (e.m_Type)
  {
    case ezAssetProcessorEvent::Type::ProcessTaskStateChanged:
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
