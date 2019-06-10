#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>
#include <EditorFramework/Panels/LongOpsPanel/LongOpsPanel.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QProgressBar>
#include <QSettings>

EZ_IMPLEMENT_SINGLETON(ezQtLongOpsPanel);

constexpr int COL_DOCUMENT = 0;
constexpr int COL_OPERATION = 1;
constexpr int COL_PROGRESS = 2;
constexpr int COL_DURATION = 3;
constexpr int COL_BUTTON = 4;

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
    header.push_back("Document");
    header.push_back("Operation");
    header.push_back("Progress");
    header.push_back("Duration");
    header.push_back(""); // Start / Cancel button

    OperationsTable->setColumnCount(header.size());
    OperationsTable->setHorizontalHeaderLabels(header);

    OperationsTable->horizontalHeader()->setStretchLastSection(false);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_DOCUMENT, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_OPERATION, QHeaderView::ResizeMode::ResizeToContents);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_PROGRESS, QHeaderView::ResizeMode::Stretch);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_DURATION, QHeaderView::ResizeMode::Fixed);
    OperationsTable->horizontalHeader()->setSectionResizeMode(COL_BUTTON, QHeaderView::ResizeMode::Fixed);

    connect(OperationsTable, &QTableWidget::cellDoubleClicked, this, &ezQtLongOpsPanel::OnCellDoubleClicked);
  }

  ezLongOpControllerManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));

  RebuildTable();
}

ezQtLongOpsPanel::~ezQtLongOpsPanel()
{
  ezLongOpControllerManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtLongOpsPanel::LongOpsEventHandler, this));
}

void ezQtLongOpsPanel::LongOpsEventHandler(const ezLongOpControllerEvent& e)
{
  if (m_bRebuildTable)
    return;

  if (e.m_Type != ezLongOpControllerEvent::Type::OpProgress)
  {
    m_bRebuildTable = true;
    return;
  }

  auto* opMan = ezLongOpControllerManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);
  m_EventQueue.PushBack(e);
}

void ezQtLongOpsPanel::RebuildTable()
{
  m_bRebuildTable = false;

  ezQtScopedBlockSignals _1(OperationsTable);

  OperationsTable->setRowCount(0);
  m_LongOpGuidToRow.Clear();

  auto* opMan = ezLongOpControllerManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  const auto& opsList = opMan->GetOperations();
  for (ezUInt32 idx = 0; idx < opsList.GetCount(); ++idx)
  {
    const auto& opInfo = *opsList[idx];
    const int rowIdx = OperationsTable->rowCount();
    OperationsTable->setRowCount(rowIdx + 1);

    // document name
    {
      ezStringBuilder docName = ezPathUtils::GetFileName(ezDocumentManager::GetDocumentByGuid(opInfo.m_DocumentGuid)->GetDocumentPath());

      OperationsTable->setItem(rowIdx, COL_DOCUMENT, new QTableWidgetItem(docName.GetData()));
    }

    // operation name
    {
      OperationsTable->setItem(rowIdx, COL_OPERATION, new QTableWidgetItem(opInfo.m_pProxyOp->GetDisplayName()));
    }

    // progress bar
    {
      QProgressBar* pProgress = new QProgressBar();
      pProgress->setValue((int)(opInfo.m_fCompletion * 100.0f));
      OperationsTable->setCellWidget(rowIdx, COL_PROGRESS, pProgress);
    }

    // duration
    {
      ezTime duration = opInfo.m_StartOrDuration;

      if (opInfo.m_bIsRunning)
        duration = ezTime::Now() - opInfo.m_StartOrDuration;

      OperationsTable->setItem(rowIdx, COL_DURATION, new QTableWidgetItem(QString("%1 sec").arg(duration.GetSeconds())));
    }

    // button
    {
      QPushButton* pButton = new QPushButton(opInfo.m_bIsRunning ? "Cancel" : "Start");
      pButton->setProperty("opGuid", QVariant::fromValue(opInfo.m_OperationGuid));

      OperationsTable->setCellWidget(rowIdx, COL_BUTTON, pButton);
      connect(pButton, &QPushButton::clicked, this, &ezQtLongOpsPanel::OnClickButton);
    }

    m_LongOpGuidToRow[opInfo.m_OperationGuid] = rowIdx;
  }
}

void ezQtLongOpsPanel::HandleEventQueue()
{
  if (!m_bRebuildTable && m_EventQueue.IsEmpty())
    return;

  auto* opMan = ezLongOpControllerManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  if (m_bRebuildTable)
  {
    m_EventQueue.Clear();
    RebuildTable();
    return;
  }

  for (const auto& e : m_EventQueue)
  {
    if (e.m_Type == ezLongOpControllerEvent::Type::OpProgress)
    {
      const auto* pOpInfo = opMan->GetOperation(e.m_OperationGuid);

      if (pOpInfo == nullptr)
        continue;

      ezUInt32 rowIdx;
      if (m_LongOpGuidToRow.TryGetValue(e.m_OperationGuid, rowIdx))
      {
        QProgressBar* pProgress = qobject_cast<QProgressBar*>(OperationsTable->cellWidget(rowIdx, COL_PROGRESS));
        pProgress->setValue((int)(pOpInfo->m_fCompletion * 100.0f));

        QPushButton* pButton = qobject_cast<QPushButton*>(OperationsTable->cellWidget(rowIdx, COL_BUTTON));
        pButton->setText(pOpInfo->m_bIsRunning ? "Cancel" : "Start");

        ezTime duration = pOpInfo->m_StartOrDuration;

        if (pOpInfo->m_bIsRunning)
          duration = ezTime::Now() - pOpInfo->m_StartOrDuration;

        OperationsTable->setItem(rowIdx, COL_DURATION, new QTableWidgetItem(QString("%1 sec").arg(duration.GetSeconds())));
      }
    }
  }

  m_EventQueue.Clear();

  if (m_LongOpGuidToRow.IsEmpty())
  {
    // reduce timer frequency when nothing is happening
    m_HandleQueueTimer.setInterval(500);
  }
  else
  {
    // increase timer frequency when stuff is going on
    m_HandleQueueTimer.setInterval(50);
  }
}

void ezQtLongOpsPanel::OnClickButton(bool)
{
  auto* opMan = ezLongOpControllerManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  QPushButton* pButton = qobject_cast<QPushButton*>(sender());
  const ezUuid opGuid = pButton->property("opGuid").value<ezUuid>();

  if (pButton->text() == "Cancel")
    opMan->CancelOperation(opGuid);
  else
    opMan->StartOperation(opGuid);
}

void ezQtLongOpsPanel::OnCellDoubleClicked(int row, int column)
{
  QPushButton* pButton = qobject_cast<QPushButton*>(OperationsTable->cellWidget(row, COL_BUTTON));
  const ezUuid opGuid = pButton->property("opGuid").value<ezUuid>();

  auto* opMan = ezLongOpControllerManager::GetSingleton();
  EZ_LOCK(opMan->m_Mutex);

  auto opInfoPtr = opMan->GetOperation(opGuid);
  if (opInfoPtr == nullptr)
    return;

  ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(opInfoPtr->m_DocumentGuid);
  pDoc->EnsureVisible();

  ezDocumentObject* pObj = pDoc->GetObjectManager()->GetObject(opInfoPtr->m_ComponentGuid);

  pDoc->GetSelectionManager()->SetSelection(pObj->GetParent());
}
