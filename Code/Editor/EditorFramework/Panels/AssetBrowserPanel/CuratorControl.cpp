#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>

ezQtCuratorControl::ezQtCuratorControl(QWidget* pParent)
  : QWidget(pParent)

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
  colors[ezAssetInfo::TransformState::Unknown] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Gray));
  colors[ezAssetInfo::TransformState::NeedsImport] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Yellow));
  colors[ezAssetInfo::TransformState::NeedsTransform] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Blue));
  colors[ezAssetInfo::TransformState::NeedsThumbnail] = ezToQtColor(ezColorScheme::DarkUI(float(ezColorScheme::Blue + ezColorScheme::Green) * 0.5f * ezColorScheme::s_fIndexNormalizer));
  colors[ezAssetInfo::TransformState::UpToDate] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Green));
  colors[ezAssetInfo::TransformState::MissingTransformDependency] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Red));
  colors[ezAssetInfo::TransformState::MissingPackageDependency] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Orange));
  colors[ezAssetInfo::TransformState::MissingThumbnailDependency] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Orange));
  colors[ezAssetInfo::TransformState::CircularDependency] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Red));
  colors[ezAssetInfo::TransformState::TransformError] = ezToQtColor(ezColorScheme::DarkUI(ezColorScheme::Red));

  const ezUInt32 uiProblems = sections[ezAssetInfo::TransformState::MissingTransformDependency] + sections[ezAssetInfo::TransformState::MissingThumbnailDependency] + sections[ezAssetInfo::TransformState::MissingPackageDependency] + sections[ezAssetInfo::TransformState::TransformError] + sections[ezAssetInfo::TransformState::CircularDependency];

  if (uiProblems > 0)
  {
    for (auto& col : colors)
    {
      col.setRed(255);
      col.setGreen(ezMath::Min(50, col.green()));
      col.setBlue(ezMath::Min(50, col.blue()));
    }
  }

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

  if (uiProblems > 0)
  {
    s.SetFormat("{} Problems (click)", uiProblems);
  }
  else
  {
    s = "Asset Status";
  }

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void ezQtCuratorControl::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

  ezQtAssetCuratorPanel::GetSingleton()->EnsureVisible();
}

void ezQtCuratorControl::UpdateBackgroundProcessState()
{
  ezAssetProcessor::ProcessTaskState state = ezAssetProcessor::GetSingleton()->GetProcessTaskState();
  switch (state)
  {
    case ezAssetProcessor::ProcessTaskState::Stopped:
      m_pBackgroundProcess->setToolTip("Start background asset processing");
      m_pBackgroundProcess->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart.svg"));
      break;
    case ezAssetProcessor::ProcessTaskState::Running:
      m_pBackgroundProcess->setToolTip("Stop background asset processing");
      m_pBackgroundProcess->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause.svg"));
      break;
    case ezAssetProcessor::ProcessTaskState::Stopping:
      m_pBackgroundProcess->setToolTip("Force stop background asset processing");
      m_pBackgroundProcess->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingForceStop.svg"));
      break;
    default:
      break;
  }

  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(state == ezAssetProcessor::ProcessTaskState::Running);
}

void ezQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  ezAssetProcessor::ProcessTaskState state = ezAssetProcessor::GetSingleton()->GetProcessTaskState();

  if (state == ezAssetProcessor::ProcessTaskState::Stopped)
  {
    ezAssetCurator::GetSingleton()->CheckFileSystem();
    ezAssetProcessor::GetSingleton()->StartProcessTask();
  }
  else
  {
    bool bForce = state == ezAssetProcessor::ProcessTaskState::Stopping;
    ezAssetProcessor::GetSingleton()->StopProcessTask(bForce);
  }
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
    s.SetFormat("Unknown: {}\nImport Needed: {}\nTransform Needed: {}\nThumbnail Needed: {}\nMissing Dependency: {}\nCircular Dependency: {}\nFailed Transform: {}",
      sections[ezAssetInfo::TransformState::Unknown],
      sections[ezAssetInfo::TransformState::NeedsImport],
      sections[ezAssetInfo::TransformState::NeedsTransform],
      sections[ezAssetInfo::TransformState::NeedsThumbnail],
      sections[ezAssetInfo::TransformState::MissingTransformDependency] + sections[ezAssetInfo::TransformState::MissingThumbnailDependency] + sections[ezAssetInfo::TransformState::MissingPackageDependency],
      sections[ezAssetInfo::TransformState::CircularDependency],
      sections[ezAssetInfo::TransformState::TransformError]);
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
      QMetaObject::invokeMethod(this, "UpdateBackgroundProcessState", Qt::QueuedConnection);
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
