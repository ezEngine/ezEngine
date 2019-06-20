#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>

ezQtAssetCuratorFilter::ezQtAssetCuratorFilter(QObject* pParent)
    : ezQtAssetFilter(pParent)
{
}

bool ezQtAssetCuratorFilter::IsAssetFiltered(const ezSubAsset* pInfo) const
{
  return !pInfo->m_bMainAsset || !(pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::MissingDependency ||
                                   pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::MissingReference ||
                                   pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::TransformError);
}

bool ezQtAssetCuratorFilter::Less(const ezSubAsset* pInfoA, const ezSubAsset* pInfoB) const
{
  // TODO: We can't sort on mutable data here as it destroys the set order, need to add a sorting model on top.
  // if (pInfoA->m_pAssetInfo->m_TransformState != pInfoB->m_pAssetInfo->m_TransformState)
  //  return pInfoA->m_pAssetInfo->m_TransformState < pInfoB->m_pAssetInfo->m_TransformState;

  ezStringView sSortA = pInfoA->GetName();
  ezStringView sSortB = pInfoB->GetName();

  ezInt32 iValue = ezStringUtils::Compare_NoCase(sSortA.GetStartPointer(), sSortB.GetStartPointer(), sSortA.GetEndPointer(), sSortB.GetEndPointer());
  if (iValue == 0)
  {
    return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
  }
  return iValue < 0;
}

EZ_IMPLEMENT_SINGLETON(ezQtAssetCuratorPanel);

ezQtAssetCuratorPanel::ezQtAssetCuratorPanel()
    : ezQtApplicationPanel("Panel.AssetCurator")
    , m_SingletonRegistrar(this)
{
  setupUi(this);

  ezAssetProcessor::GetSingleton()->AddLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));

  m_pFilter = new ezQtAssetCuratorFilter(this);
  m_pModel = new ezQtAssetBrowserModel(this, m_pFilter);
  m_pModel->SetIconMode(false);

  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  EZ_VERIFY(connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                    &ezQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this,
                    [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
                      if (m_selectedIndex.isValid() && topLeft.row() <= m_selectedIndex.row() && m_selectedIndex.row() <= bottomRight.row())
                      {
                        UpdateIssueInfo();
                      }
                    }),
            "signal/slot connection failed");

  EZ_VERIFY(connect(m_pModel, &QAbstractItemModel::modelReset, this,
                    [this]() {
                      m_selectedIndex = QPersistentModelIndex();
                      UpdateIssueInfo();
                    }),
            "signal/slot connection failed");
}

ezQtAssetCuratorPanel::~ezQtAssetCuratorPanel()
{
  ezAssetProcessor::GetSingleton()->RemoveLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));
}

void ezQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.isEmpty())
    m_selectedIndex = QModelIndex();
  else
    m_selectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void ezQtAssetCuratorPanel::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
  QString sAbsPath = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

void ezQtAssetCuratorPanel::LogWriter(const ezLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  ezLogEntry msg(e);
  CuratorLog->GetLog()->AddLogMsg(msg);
}

void ezQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_selectedIndex.isValid())
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  ezUuid assetGuid = m_pModel->data(m_selectedIndex, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  ezAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [](const ezString& dep) -> ezStringBuilder {
    if (ezConversionUtils::IsStringUuid(dep))
    {
      auto assetInfoDep = ezAssetCurator::GetSingleton()->GetSubAsset(ezConversionUtils::ConvertStringToUuid(dep));
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_sDataDirRelativePath;
      }
    }

    return dep;
  };

  ezLogEntryDelegate logger(([this](ezLogEntry& entry) -> void { TransformLog->GetLog()->AddLogMsg(std::move(entry)); }));
  ezStringBuilder text;
  if (pAssetInfo->m_TransformState == ezAssetInfo::MissingDependency)
  {
    ezLog::Error(&logger, "Missing Dependency:");
    for (const ezString& dep : pAssetInfo->m_MissingDependencies)
    {
      ezStringBuilder sNiceName = getNiceName(dep);
      ezLog::Error(&logger, "{0}", sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == ezAssetInfo::MissingReference)
  {
    ezLog::Error(&logger, "Missing Reference:");
    for (const ezString& ref : pAssetInfo->m_MissingReferences)
    {
      ezStringBuilder sNiceName = getNiceName(ref);
      ezLog::Error(&logger, "{0}", sNiceName);
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
