#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>

ezQtAssetCuratorFilter::ezQtAssetCuratorFilter(QObject* pParent)
  : ezQtAssetFilter(pParent)
{
}

void ezQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool ezQtAssetCuratorFilter::IsAssetFiltered(ezStringView sDataDirParentRelativePath, bool bIsFolder, const ezSubAsset* pInfo) const
{
  if (!pInfo)
    return true;

  if (!pInfo->m_bMainAsset)
    return true;

  if ((pInfo->m_pAssetInfo->m_TransformState != ezAssetInfo::MissingTransformDependency) && (pInfo->m_pAssetInfo->m_TransformState != ezAssetInfo::CircularDependency) && (pInfo->m_pAssetInfo->m_TransformState != ezAssetInfo::MissingThumbnailDependency) && (pInfo->m_pAssetInfo->m_TransformState != ezAssetInfo::MissingPackageDependency) && (pInfo->m_pAssetInfo->m_TransformState != ezAssetInfo::TransformError))
  {
    return true;
  }

  if (m_bFilterTransitive)
  {
    if (pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::MissingThumbnailDependency)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingThumbnailDeps)
      {
        if (!ezAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        {
          return false;
        }
      }

      return true;
    }

    if (pInfo->m_pAssetInfo->m_TransformState == ezAssetInfo::MissingPackageDependency)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingPackageDeps)
      {
        if (!ezAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        {
          return false;
        }
      }

      return true;
    }
  }

  return false;
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

  ezAssetProcessor::GetSingleton()->AddLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));

  m_pFilter = new ezQtAssetCuratorFilter(this);
  m_pModel = new ezQtAssetBrowserModel(this, m_pFilter);
  m_pModel->SetIconMode(false);

  TransformLog->ShowControls(false);

  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  EZ_VERIFY(
    connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ezQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
    "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this,
              [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
              {
                if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                {
                  UpdateIssueInfo();
                }
              }),
    "signal/slot connection failed");

  EZ_VERIFY(connect(m_pModel, &QAbstractItemModel::modelReset, this,
              [this]()
              {
                m_SelectedIndex = QPersistentModelIndex();
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
    m_SelectedIndex = QModelIndex();
  else
    m_SelectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void ezQtAssetCuratorPanel::onListAssetsDoubleClicked(const QModelIndex& index)
{
  QString sAbsPath = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

void ezQtAssetCuratorPanel::onCheckIndirectToggled(bool checked)
{
  m_pFilter->SetFilterTransitive(!checked);
  m_pModel->resetModel();
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

  ezUuid assetGuid = m_pModel->data(m_SelectedIndex, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  ezAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [](const ezString& sDep) -> ezStringBuilder
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
      ezStringBuilder sTmp;
      sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

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
