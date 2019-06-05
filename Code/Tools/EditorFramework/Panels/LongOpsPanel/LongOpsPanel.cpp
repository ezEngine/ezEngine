#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QProgressBar>
#include <QSettings>

EZ_IMPLEMENT_SINGLETON(ezQtLongOpsPanel);

ezQtLongOpsPanel ::ezQtLongOpsPanel()
  : ezQtApplicationPanel("Panel.Log")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  setWindowIcon(ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Log.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.LongOps")));

  // TODO: find a better way to do a queued UI update when events are available
  connect(&m_HandleQueueTimer, &QTimer::timeout, this, &ezQtLongOpsPanel::HandleEventQueue, Qt::QueuedConnection);
  m_HandleQueueTimer.setInterval(200);
  m_HandleQueueTimer.start();

  // setup table
  {
    QStringList header;
    header.push_back("Operation");
    header.push_back("Progress");

    OperationsTable->setColumnCount(header.size());
    OperationsTable->setHorizontalHeaderLabels(header);

    OperationsTable->horizontalHeader()->setStretchLastSection(false);
    OperationsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
  }

  ezLongOperationManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LongOpsPanel"));
  {
    // splitter->restoreState(Settings.value("Splitter", splitter->saveState()).toByteArray());
  }
  Settings.endGroup();
}

ezQtLongOpsPanel::~ezQtLongOpsPanel()
{
  ezLongOperationManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));

  QSettings Settings;
  Settings.beginGroup(QLatin1String("LongOpsPanel"));
  {
    // Settings.setValue("Splitter", splitter->saveState());
  }
  Settings.endGroup();
}

void ezQtLongOpsPanel::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectClosing:
    {

      [[fallthrough]];
    }

    case ezToolsProjectEvent::Type::ProjectOpened:
    {
      setEnabled(e.m_Type == ezToolsProjectEvent::Type::ProjectOpened);
      break;
    }
  }

  ezQtApplicationPanel::ToolsProjectEventHandler(e);
}

void ezQtLongOpsPanel::LongOpsEventHandler(const ezLongOperationManagerEvent& e)
{
  auto* opMan = ezLongOperationManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  m_EventQueue.PushBack(e);
}

void ezQtLongOpsPanel::HandleEventQueue()
{
  if (m_EventQueue.IsEmpty())
    return;

  auto* opMan = ezLongOperationManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  for (const auto& e : m_EventQueue)
  {
    const auto& opInfo = opMan->GetOperations()[e.m_uiOperationIndex];

    if (e.m_Type == ezLongOperationManagerEvent::Type::OpAdded)
    {
      const int rowIdx = OperationsTable->rowCount();
      OperationsTable->setRowCount(rowIdx + 1);
      OperationsTable->setItem(rowIdx, 0, new QTableWidgetItem(opInfo.m_pOperation->GetDisplayName()));
      OperationsTable->setCellWidget(rowIdx, 1, new QProgressBar());
    }
    else if (e.m_Type == ezLongOperationManagerEvent::Type::OpProgress)
    {
      const int rowIdx = e.m_uiOperationIndex;
      QProgressBar* pProgress = qobject_cast<QProgressBar*>(OperationsTable->cellWidget(rowIdx, 1));
      pProgress->setValue((int)(opInfo.m_fCompletion * 100.0f));
    }
    else if (e.m_Type == ezLongOperationManagerEvent::Type::OpFinished)
    {
      const int rowIdx = e.m_uiOperationIndex;
      QProgressBar* pProgress = qobject_cast<QProgressBar*>(OperationsTable->cellWidget(rowIdx, 1));
      pProgress->setValue(100);
    }
  }

  m_EventQueue.Clear();
}
