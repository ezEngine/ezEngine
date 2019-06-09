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
  m_HandleQueueTimer.setInterval(500);
  m_HandleQueueTimer.start();

  // setup table
  {
    QStringList header;
    header.push_back("Operation");
    header.push_back("Progress");
    header.push_back("Duration");
    header.push_back(""); // Cancel

    OperationsTable->setColumnCount(header.size());
    OperationsTable->setHorizontalHeaderLabels(header);

    OperationsTable->horizontalHeader()->setStretchLastSection(false);
    OperationsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    OperationsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::Fixed);
    OperationsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeMode::Fixed);
  }

  ezLongOpManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));
}

ezQtLongOpsPanel::~ezQtLongOpsPanel()
{
  ezLongOpManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));
}

void ezQtLongOpsPanel::LongOpsEventHandler(const ezLongOpManagerEvent& e)
{
  auto* opMan = ezLongOpManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  m_EventQueue.PushBack(e);
}

void ezQtLongOpsPanel::RebuildTable()
{
  ezQtScopedBlockSignals _1(OperationsTable);

  OperationsTable->setRowCount(0);
  m_LongOpIdxToRow.Clear();

  auto* opMan = ezLongOpManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  const auto& opsList = opMan->GetOperations();
  for (ezUInt32 idx = 0; idx < opsList.GetCount(); ++idx)
  {
    const auto& opInfo = *opsList[idx];

    if (opInfo.m_pOperation == nullptr)
      continue;

    const int rowIdx = OperationsTable->rowCount();
    OperationsTable->setRowCount(rowIdx + 1);
    OperationsTable->setItem(rowIdx, 0, new QTableWidgetItem(opInfo.m_pOperation->GetDisplayName()));

    QProgressBar* pProgress = new QProgressBar();
    pProgress->setValue((int)(opInfo.m_Progress.GetCompletion() * 100.0f));

    OperationsTable->setCellWidget(rowIdx, 1, pProgress);
    OperationsTable->setItem(
      rowIdx, 2, new QTableWidgetItem(QString("%1 sec").arg((ezTime::Now() - opInfo.m_StartOrDuration).GetSeconds())));

    QPushButton* pButton = new QPushButton("Cancel");
    pButton->setProperty("opIdx", idx);

    OperationsTable->setCellWidget(rowIdx, 3, pButton);
    connect(pButton, &QPushButton::clicked, this, &ezQtLongOpsPanel::OnClickCancel);

    m_LongOpIdxToRow[idx] = rowIdx;
  }
}

void ezQtLongOpsPanel::HandleEventQueue()
{
  if (m_EventQueue.IsEmpty())
    return;

  auto* opMan = ezLongOpManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  for (const auto& e : m_EventQueue)
  {
    const auto& opInfo = *opMan->GetOperations()[e.m_uiOperationIndex];

    if (e.m_Type == ezLongOpManagerEvent::Type::OpAdded || e.m_Type == ezLongOpManagerEvent::Type::OpFinished)
    {
      // if anything was added or removed, recreate the entire table, then break
      RebuildTable();
      break;
    }
    else if (e.m_Type == ezLongOpManagerEvent::Type::OpProgress)
    {
      ezUInt32 rowIdx;
      if (m_LongOpIdxToRow.TryGetValue(e.m_uiOperationIndex, rowIdx))
      {
        QProgressBar* pProgress = qobject_cast<QProgressBar*>(OperationsTable->cellWidget(rowIdx, 1));
        pProgress->setValue((int)(opInfo.m_Progress.GetCompletion() * 100.0f));

        OperationsTable->setItem(
          rowIdx, 2, new QTableWidgetItem(QString("%1 sec").arg((ezTime::Now() - opInfo.m_StartOrDuration).GetSeconds())));
      }
    }
  }

  if (m_LongOpIdxToRow.IsEmpty())
  {
    // reduce timer frequency when nothing is happening
    m_HandleQueueTimer.setInterval(500);
  }
  else
  {
    // increase timer frequency when stuff is going on
    m_HandleQueueTimer.setInterval(50);
  }

  m_EventQueue.Clear();
}

void ezQtLongOpsPanel::OnClickCancel(bool)
{
  auto* opMan = ezLongOpManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  QPushButton* pButton = qobject_cast<QPushButton*>(sender());
  const int opIdx = pButton->property("opIdx").toInt();

  opMan->CancelOperation(opIdx);
}
