#include <PCH.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <EditorFramework/Panels/LogPanel/LogModel.moc.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetCurator.h>

ezQtAssetCuratorFilter::ezQtAssetCuratorFilter(QObject* pParent)
  : ezQtAssetFilter(pParent)
{
}

bool ezQtAssetCuratorFilter::IsAssetFiltered(const ezAssetInfo* pInfo) const
{
  return !(pInfo->m_TransformState == ezAssetInfo::MissingDependency || pInfo->m_TransformState == ezAssetInfo::MissingReference
    || pInfo->m_TransformState == ezAssetInfo::TransformError);
}

bool ezQtAssetCuratorFilter::Less(ezAssetInfo* pInfoA, ezAssetInfo* pInfoB) const
{
  if (pInfoA->m_TransformState != pInfoB->m_TransformState)
    return pInfoA->m_TransformState < pInfoB->m_TransformState;

  ezStringView sSortA = ezPathUtils::GetFileName(pInfoA->m_sDataDirRelativePath.GetData(), pInfoA->m_sDataDirRelativePath.GetData() + pInfoA->m_sDataDirRelativePath.GetElementCount());
  ezStringView sSortB = ezPathUtils::GetFileName(pInfoB->m_sDataDirRelativePath.GetData(), pInfoB->m_sDataDirRelativePath.GetData() + pInfoB->m_sDataDirRelativePath.GetElementCount());

  ezInt32 iValue = ezStringUtils::Compare_NoCase(sSortA.GetData(), sSortB.GetData(),
    sSortA.GetData() + sSortA.GetElementCount(), sSortB.GetData() + sSortB.GetElementCount());
  if (iValue == 0)
  {
    return pInfoA->m_Info.m_DocumentID < pInfoB->m_Info.m_DocumentID;
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

  IssueInfo->setReadOnly(true);
  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  EZ_VERIFY(connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ezQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
  {
    if (m_selectedIndex.isValid() && topLeft.row() <= m_selectedIndex.row() && m_selectedIndex.row() <= bottomRight.row())
    {
      UpdateIssueInfo();
    }
  }), "signal/slot connection failed");
}

ezQtAssetCuratorPanel::~ezQtAssetCuratorPanel()
{
  ezAssetProcessor::GetSingleton()->RemoveLogWriter(ezMakeDelegate(&ezQtAssetCuratorPanel::LogWriter, this));
}

void ezQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  if (selected.isEmpty())
    m_selectedIndex = QModelIndex();
  else
    m_selectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void ezQtAssetCuratorPanel::LogWriter(const ezLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  ezQtLogModel::LogMsg msg;
  msg.m_sMsg = e.m_szText;
  msg.m_sTag = e.m_szTag;
  msg.m_Type = e.m_EventType;
  msg.m_uiIndentation = e.m_uiIndentation;

  CuratorLog->GetLog()->AddLogMsg(msg);
}

void ezQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_selectedIndex.isValid())
  {
    IssueInfo->clear();
    return;
  }

  ezUuid assetGuid = m_pModel->data(m_selectedIndex, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
  auto assetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(assetGuid);
  if (assetInfo == nullptr)
  {
    IssueInfo->clear();
    return;
  }

  auto getNiceName = [](const ezString& dep) -> ezStringBuilder
  {
    if (ezConversionUtils::IsStringUuid(dep))
    {
      auto assetInfoDep = ezAssetCurator::GetSingleton()->GetAssetInfo2(ezConversionUtils::ConvertStringToUuid(dep));
      if (assetInfoDep)
      {
        return assetInfoDep->m_sDataDirRelativePath;
      }
    }

    return dep;
  };

  ezStringBuilder text;
  if (assetInfo->m_TransformState == ezAssetInfo::MissingDependency)
  {
    text = "<span style=\"color:#ff8800;\">Missing Dependency:</span><br><br>";
    for (const ezString& dep : assetInfo->m_MissingDependencies)
    {
      ezStringBuilder sNiceName = getNiceName(dep);
      text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", sNiceName);
    }
  }
  else if (assetInfo->m_TransformState == ezAssetInfo::MissingReference)
  {
    text = "<span style=\"color:#ff8800;\">Missing Reference:</span><br><br>";
    for (const ezString& ref : assetInfo->m_MissingReferences)
    {
      ezStringBuilder sNiceName = getNiceName(ref);
      text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", sNiceName);
    }
  }
  else if (assetInfo->m_TransformState == ezAssetInfo::TransformError)
  {
    text = "<span style=\"color:#ff8800;\">Transform Error:</span><br><br>";
    text.AppendFormat("<span style=\"color:#ffaa00;\">{0}</span><br>", "More info coming soon!");
  }

  IssueInfo->setAcceptRichText(true);
  IssueInfo->setHtml(text.GetData());
}
