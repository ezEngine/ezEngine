#include <EditorFramework/Assets/AssetDocument.h>

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <QMenu>
#include <QTimer>

ezQtAssetCuratorFilter::ezQtAssetCuratorFilter(QObject* pParent)
  : ezQtAssetFilter(pParent)
{
}

void ezQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool ezQtAssetCuratorFilter::HasIssue(const ezSubAsset* pInfo)
{
  if (!pInfo)
    return false;

  if (!pInfo->m_bMainAsset)
    return false;

  const ezAssetInfo::TransformState state = pInfo->m_pAssetInfo->m_TransformState;

  return state == ezAssetInfo::MissingTransformDependency || state == ezAssetInfo::CircularDependency || state == ezAssetInfo::MissingThumbnailDependency || state == ezAssetInfo::MissingPackageDependency || state == ezAssetInfo::TransformError;
}

bool ezQtAssetCuratorFilter::IsIndirectIssue(const ezSubAsset* pInfo)
{
  // An issue is 'indirect' when every dependency that this asset is missing resolves to an asset
  // that the curator still knows about. Those assets are reported with their own issue, so listing
  // everything downstream of them would only repeat the same root cause.
  auto allDepsResolve = [](const ezSet<ezString>& deps) -> bool
  {
    for (const ezString& ref : deps)
    {
      if (!ezAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        return false;
    }
    return true;
  };

  switch (pInfo->m_pAssetInfo->m_TransformState)
  {
    case ezAssetInfo::MissingThumbnailDependency:
      return allDepsResolve(pInfo->m_pAssetInfo->m_MissingThumbnailDeps);

    case ezAssetInfo::MissingPackageDependency:
      return allDepsResolve(pInfo->m_pAssetInfo->m_MissingPackageDeps);

    default:
      return false;
  }
}

ezAssetFilterResult ezQtAssetCuratorFilter::IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const
{
  if (!HasIssue(pInfo))
    return ezAssetFilterResult::Filtered;

  if (m_bFilterTransitive && IsIndirectIssue(pInfo))
    return ezAssetFilterResult::Filtered;

  return ezAssetFilterResult::Visible;
}

EZ_IMPLEMENT_SINGLETON(ezQtAssetCuratorPanel);

ezQtAssetCuratorPanel::ezQtAssetCuratorPanel(ads::CDockManager* pDockManager)
  : ezQtApplicationPanel(pDockManager, "Panel.AssetCurator")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  // using pDummy instead of 'this' breaks auto-connect for slots
  setWidget(pDummy);
  setIcon(ezQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/AssetCurator.svg"));
  setWindowTitle(ezMakeQString(ezTranslate("Panel.AssetCurator")));

  connect(ListAssets, &QTreeView::doubleClicked, this, &ezQtAssetCuratorPanel::onListAssetsDoubleClicked);
  connect(CheckIndirect, &QCheckBox::toggled, this, &ezQtAssetCuratorPanel::onCheckIndirectToggled);
  connect(ListAssets, &QWidget::customContextMenuRequested, this, &ezQtAssetCuratorPanel::onListAssetsContextMenuRequested);

  ezAssetProcessor::GetSingleton()->AddLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));

  ProcessorProgress->SetGridBarWidget(ProcessorProgressGridBar);
  ProcessorProgress->SetScrollBarWidget(ProcessorScrollBar);

  m_pFilter = new ezQtAssetCuratorFilter(this);
  m_Model = QSharedPointer<ezQtAssetBrowserModel>(new ezQtAssetBrowserModel(this, m_pFilter));
  m_Model->Initialize();
  m_Model->SetIconMode(false);

  TransformLog->ShowControls(false);

  CuratorLog->setVisible(false);

  ListAssets->setModel(m_Model.data());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  EZ_VERIFY(
    connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ezQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
    "signal/slot connection failed");
  EZ_VERIFY(connect(m_Model.data(), &QAbstractItemModel::dataChanged, this,
              [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
              {
                if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                {
                  UpdateIssueInfo();
                }
              }),
    "signal/slot connection failed");

  EZ_VERIFY(connect(m_Model.data(), &QAbstractItemModel::modelReset, this,
              [this]()
              {
                m_SelectedIndex = QPersistentModelIndex();
                UpdateIssueInfo();
                UpdateIndirectIssueCount();
              }),
    "signal/slot connection failed");

  // An asset can become (or stop being) an indirect issue without ever entering the list, so the
  // model's own signals are not enough to keep the count current - listen to the curator instead.
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetCuratorPanel::AssetCuratorEventHandler, this));

  UpdateIndirectIssueCount();

  EZ_VERIFY(connect(ClearHistory, &QToolButton::clicked, ProcessorProgress, &ezQtAssetProcessorProgressWidget::ClearHistory), "");
}

ezQtAssetCuratorPanel::~ezQtAssetCuratorPanel()
{
  ezAssetProcessor::GetSingleton()->RemoveLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetCuratorPanel::AssetCuratorEventHandler, this));
}

void ezQtAssetCuratorPanel::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case ezAssetCuratorEvent::Type::AssetAdded:
    case ezAssetCuratorEvent::Type::AssetMoved:
    case ezAssetCuratorEvent::Type::AssetRemoved:
    case ezAssetCuratorEvent::Type::AssetUpdated:
    case ezAssetCuratorEvent::Type::AssetListReset:
      ScheduleIndirectIssueCountUpdate();
      break;

    default:
      break;
  }
}

void ezQtAssetCuratorPanel::ScheduleIndirectIssueCountUpdate()
{
  // Curator events arrive in bursts, so coalesce them - recounting walks all known assets.
  if (m_bIndirectCountScheduled)
    return;

  m_bIndirectCountScheduled = true;

  QTimer::singleShot(200, this, [this]()
    {
      m_bIndirectCountScheduled = false;
      UpdateIndirectIssueCount(); //
    });
}

void ezQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.isEmpty())
    m_SelectedIndex = QModelIndex();
  else
    m_SelectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void ezQtAssetCuratorPanel::onListAssetsDoubleClicked(const QModelIndex& index)
{
  QString sAbsPath = m_Model->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

QModelIndex ezQtAssetCuratorPanel::GetContextMenuTarget() const
{
  return ListAssets->selectionModel()->currentIndex();
}

void ezQtAssetCuratorPanel::onListAssetsContextMenuRequested(const QPoint& pos)
{
  // make the item under the cursor the current one, so that the menu always acts on what was clicked
  const QModelIndex clicked = ListAssets->indexAt(pos);
  if (clicked.isValid())
  {
    ListAssets->selectionModel()->setCurrentIndex(clicked, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }

  const QModelIndex index = GetContextMenuTarget();
  if (!index.isValid())
    return;

  QMenu menu;

  QAction* pOpen = menu.addAction(ezMakeQString(ezTranslate("AssetCurator.OpenDocument")));
  connect(pOpen, &QAction::triggered, this, [this]()
    {
      const QModelIndex idx = GetContextMenuTarget();
      if (idx.isValid())
        onListAssetsDoubleClicked(idx); //
    });

  QAction* pSelect = menu.addAction(ezMakeQString(ezTranslate("AssetCurator.SelectInAssetBrowser")));
  connect(pSelect, &QAction::triggered, this, [this]()
    {
      const QModelIndex idx = GetContextMenuTarget();
      if (!idx.isValid())
        return;

      const ezUuid assetGuid = m_Model->data(idx, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
      if (!assetGuid.IsValid())
        return;

      ezQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(assetGuid);
      ezQtAssetBrowserPanel::GetSingleton()->EnsureVisible(); //
    });

  // the double click default action is 'open', so make that the bold default entry as well
  menu.setDefaultAction(pOpen);

  menu.exec(ListAssets->viewport()->mapToGlobal(pos));
}

void ezQtAssetCuratorPanel::onCheckIndirectToggled(bool checked)
{
  m_pFilter->SetFilterTransitive(!checked);
  m_Model->resetModel();
  UpdateIndirectIssueCount();
}

void ezQtAssetCuratorPanel::UpdateIndirectIssueCount()
{
  ezUInt32 uiIndirect = 0;

  {
    ezAssetCurator::ezLockedSubAssetTable allAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it : *allAssetsLocked)
    {
      const ezSubAsset* pSubAsset = &it.Value();

      if (ezQtAssetCuratorFilter::HasIssue(pSubAsset) && ezQtAssetCuratorFilter::IsIndirectIssue(pSubAsset))
      {
        ++uiIndirect;
      }
    }
  }

  if (uiIndirect == 0)
  {
    CheckIndirect->setText("Show Indirect Issues");
  }
  else
  {
    ezStringBuilder sText;
    sText.SetFormat(uiIndirect == 1 ? "Show {} Indirect Issue" : "Show {} Indirect Issues", uiIndirect);
    CheckIndirect->setText(ezMakeQString(sText));
  }
}

void ezQtAssetCuratorPanel::LogWriter(const ezLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  ezLogEntry msg(e);
  CuratorLog->GetLog()->AddLogMsg(msg);
}

void ezQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_SelectedIndex.isValid())
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  ezUuid assetGuid = m_Model->data(m_SelectedIndex, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  ezAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [&pSubAsset](const ezString& sDep) -> ezStringBuilder
  {
    if (ezConversionUtils::IsStringUuid(sDep))
    {
      ezUuid guid = ezConversionUtils::ConvertStringToUuid(sDep);
      auto assetInfoDep = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      }

      ezUInt64 uiLow;
      ezUInt64 uiHigh;
      guid.GetValues(uiLow, uiHigh);

      ezString sDocumentPath = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath();

      // Open the document (without requesting a window)
      ezDocument* pDocument = ezQtEditorApp::GetSingleton()->OpenDocument(sDocumentPath, ezDocumentFlags::None);

      constexpr ezUInt32 maxResults = 3;
      ezTempHybridArray<ezAssetDocument::AssetUsage, maxResults> uses;
      if (pDocument != nullptr)
      {
        // cast a document to ezAssetDocument to access the FindAssetUsages function.
        if (ezAssetDocument* pAssetDoc = ezDynamicCast<ezAssetDocument*>(pDocument))
        {
          // Find all direct uses of this asset
          pAssetDoc->FindAssetUsages(sDep, uses, maxResults);
        }
      }

      ezStringBuilder sTmp;
      if (uses.IsEmpty())
      {
        sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);
      }
      else
      {
        ezStringBuilder usesString;
        for (auto& use : uses)
        {
          if (!usesString.IsEmpty())
          {
            usesString.Append(", ");
          }
          usesString.Append("'", use.m_sObjectName, "'");
        }

        sTmp.SetFormat("{}. Used by objects: [[{}|asset:{}#filter:\"ref:{}\"]]", sDep, usesString, pSubAsset->m_pAssetInfo->m_Info->m_DocumentID, sDep);
      }

      return sTmp;
    }

    return sDep;
  };

  ezLogEntryDelegate logger(([this](ezLogEntry& ref_entry) -> void
    { TransformLog->GetLog()->AddLogMsg(std::move(ref_entry)); }));
  ezStringBuilder text;
  if (pAssetInfo->m_TransformState == ezAssetInfo::MissingTransformDependency)
  {
    ezLog::Error(&logger, "Missing Transform Dependency:");
    for (const ezString& dep : pAssetInfo->m_MissingTransformDeps)
    {
      ezStringBuilder m_sNiceName = getNiceName(dep);
      ezLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == ezAssetInfo::CircularDependency)
  {
    ezLog::Error(&logger, "Circular Dependency:");
    for (const ezString& ref : pAssetInfo->m_CircularDependencies)
    {
      ezStringBuilder m_sNiceName = getNiceName(ref);
      ezLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == ezAssetInfo::MissingThumbnailDependency)
  {
    ezLog::Error(&logger, "Missing Thumbnail Dependency:");
    for (const ezString& ref : pAssetInfo->m_MissingThumbnailDeps)
    {
      ezStringBuilder m_sNiceName = getNiceName(ref);
      ezLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == ezAssetInfo::MissingPackageDependency)
  {
    ezLog::Error(&logger, "Missing Package Dependency:");
    for (const ezString& ref : pAssetInfo->m_MissingPackageDeps)
    {
      ezStringBuilder m_sNiceName = getNiceName(ref);
      ezLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == ezAssetInfo::TransformError)
  {
    ezLog::Error(&logger, "Transform Error:");
    for (const ezLogEntry& logEntry : pAssetInfo->m_LogEntries)
    {
      TransformLog->GetLog()->AddLogMsg(logEntry);
    }
  }
}
