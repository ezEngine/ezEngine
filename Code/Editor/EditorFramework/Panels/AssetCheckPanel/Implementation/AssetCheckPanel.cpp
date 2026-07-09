#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetChecker.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Panels/AssetCheckPanel/AssetCheckPanel.moc.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QPushButton>

EZ_IMPLEMENT_SINGLETON(ezQtAssetCheckPanel);

ezQtAssetCheckPanel::ezQtAssetCheckPanel(ads::CDockManager* pDockManager)
  : ezQtApplicationPanel(pDockManager, "Panel.AssetCheck")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setIcon(ezQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Checklist.svg"));
  setWindowTitle(ezMakeQString(ezTranslate("Panel.AssetCheck")));
  setWidget(pDummy);

  MainSplitter->setStretchFactor(0, 1);
  MainSplitter->setStretchFactor(1, 2);

  // The panel has no real geometry yet at construction time (ADS assigns it later), so setSizes() here would be
  // clamped to a 0-size splitter and lost. Apply the ratio on the splitter's first real resize instead.
  MainSplitter->installEventFilter(this);

  connect(RunButton, &QPushButton::clicked, this, &ezQtAssetCheckPanel::RunButtonClicked);
  connect(ResultTree, &QTreeWidget::itemDoubleClicked, this, &ezQtAssetCheckPanel::ResultTreeItemDoubleClicked);

  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetCheckPanel::DocumentManagerEventHandler, this));

  UpdateAssetTypeCombo();

  ezAssetCheckRule::CreateRules(m_Rules);
  FillRuleList();
}

ezQtAssetCheckPanel::~ezQtAssetCheckPanel()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetCheckPanel::DocumentManagerEventHandler, this));

  ezAssetCheckRule::DestroyRules(m_Rules);
}

bool ezQtAssetCheckPanel::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pWatched == MainSplitter && pEvent->type() == QEvent::Resize)
  {
    const int iTotal = MainSplitter->orientation() == Qt::Horizontal ? MainSplitter->width() : MainSplitter->height();
    if (iTotal > 0)
    {
      MainSplitter->setSizes({iTotal / 3, iTotal - (iTotal / 3)});
      MainSplitter->removeEventFilter(this);
    }
  }

  return ezQtApplicationPanel::eventFilter(pWatched, pEvent);
}

void ezQtAssetCheckPanel::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentTypesAdded || e.m_Type == ezDocumentManager::Event::Type::DocumentTypesRemoved)
  {
    UpdateAssetTypeCombo();
  }
}

void ezQtAssetCheckPanel::UpdateAssetTypeCombo()
{
  // Remember the previous selection (by asset type name) so a plugin (re-)load doesn't reset the user's choice.
  const QString sPreviouslySelected = AssetTypeCombo->currentData().toString();

  AssetTypeCombo->clear();
  AssetTypeCombo->addItem("<All Asset Types>", QString());

  ezSet<ezString> addedTypes;
  for (auto it : ezDocumentManager::GetAllDocumentDescriptors())
  {
    const ezDocumentTypeDescriptor* pDesc = it.Value();
    if (ezDynamicCast<ezAssetDocumentManager*>(pDesc->m_pManager) == nullptr)
      continue;

    if (addedTypes.Contains(pDesc->m_sDocumentTypeName))
      continue;

    addedTypes.Insert(pDesc->m_sDocumentTypeName);

    const QString sTypeName = QString::fromUtf8(pDesc->m_sDocumentTypeName.GetData());
    AssetTypeCombo->addItem(sTypeName, sTypeName);
  }

  if (const int iIndex = AssetTypeCombo->findData(sPreviouslySelected); iIndex >= 0)
    AssetTypeCombo->setCurrentIndex(iIndex);
}

void ezQtAssetCheckPanel::FillRuleList()
{
  RuleList->clear();

  for (ezUInt32 i = 0; i < m_Rules.GetCount(); ++i)
  {
    ezAssetCheckRule* pRule = m_Rules[i];

    const ezStringBuilder sText = pRule->GetDisplayName();

    QListWidgetItem* pItem = new QListWidgetItem(QString::fromUtf8(sText.GetData()), RuleList);
    pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    pItem->setCheckState(Qt::Checked);

    ezStringBuilder sTooltip(pRule->GetDescription());

    if (pRule->CanFix())
    {
      pItem->setForeground(ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green)));
      sTooltip.Append("\n\nThis rule supports auto-fix.");
    }

    pItem->setToolTip(QString::fromUtf8(sTooltip.GetData()));
    pItem->setData(Qt::UserRole, i);
  }
}

void ezQtAssetCheckPanel::RunButtonClicked()
{
  ezAssetCheckOptions options;

  const QVariant typeData = AssetTypeCombo->currentData();
  options.m_sDocumentTypeName = typeData.toString().toUtf8().data();
  options.m_sNameFilter = NameFilterEdit->text().toUtf8().data();
  options.m_bAutoFix = AutoFixCheck->isChecked();

  for (int i = 0; i < RuleList->count(); ++i)
  {
    QListWidgetItem* pItem = RuleList->item(i);
    if (pItem->checkState() != Qt::Checked)
      continue;

    const ezUInt32 uiRuleIndex = pItem->data(Qt::UserRole).toUInt();
    options.m_Rules.PushBack(m_Rules[uiRuleIndex]);
  }

  if (options.m_Rules.IsEmpty())
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("Select at least one rule to run.");
    return;
  }

  ezAssetCheckSummary summary;
  {
    // The modal progress dialog appears automatically via the global ezProgress.
    ezAssetChecker::Run(options, summary);
  }

  // Fill the results tree.
  ResultTree->clear();

  const QIcon errorIcon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Error.svg");
  const QIcon warningIcon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/Warning.svg");

  for (const ezAssetCheckResult& result : summary.m_Results)
  {
    ezUInt32 uiErrors = 0, uiWarnings = 0;
    for (const ezAssetCheckNote& note : result.m_Notes)
    {
      if (note.m_Severity == ezAssetCheckSeverity::Error)
        ++uiErrors;
      else
        ++uiWarnings;
    }

    ezStringBuilder sTitle;
    sTitle.SetFormat("{}  ({} errors, {} warnings)", result.m_sAssetPath, uiErrors, uiWarnings);

    // Link target understood by ezQtUiServices::GotoLinkTarget: "asset:<assetGuid>[#<objectGuid>]".
    ezStringBuilder sAssetGuid;
    ezConversionUtils::ToString(result.m_AssetGuid, sAssetGuid);
    ezStringBuilder sAssetLink("asset:", sAssetGuid);

    QTreeWidgetItem* pTop = new QTreeWidgetItem(ResultTree);
    pTop->setText(0, QString::fromUtf8(sTitle.GetData()));
    pTop->setIcon(0, uiErrors > 0 ? errorIcon : warningIcon);
    pTop->setData(0, Qt::UserRole, QString::fromUtf8(sAssetLink.GetData()));

    for (const ezAssetCheckNote& note : result.m_Notes)
    {
      ezStringBuilder sNote = note.m_sMessage;
      if (note.m_bFixed)
        sNote.Append(" (fixed)");

      QTreeWidgetItem* pChild = new QTreeWidgetItem(pTop);
      pChild->setText(0, QString::fromUtf8(sNote.GetData()));
      pChild->setIcon(0, note.m_Severity == ezAssetCheckSeverity::Error ? errorIcon : warningIcon);

      if (note.m_bFixed)
        pChild->setForeground(0, ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green)));

      // Append the object GUID so a double-click selects that object in the opened document.
      ezStringBuilder sNoteLink = sAssetLink;
      if (note.m_ObjectGuid.IsValid())
      {
        ezStringBuilder sObjGuid;
        ezConversionUtils::ToString(note.m_ObjectGuid, sObjGuid);
        sNoteLink.Append("#", sObjGuid);
      }
      pChild->setData(0, Qt::UserRole, QString::fromUtf8(sNoteLink.GetData()));
    }

    if (uiErrors > 0)
      pTop->setExpanded(true);
  }

  ezStringBuilder sStatus;
  if (summary.m_Results.IsEmpty())
  {
    sStatus = "No issues found.";
  }
  else
  {
    sStatus.SetFormat("Checked {} assets: {} errors, {} warnings, {} auto-fixed, {} documents saved.",
      summary.m_uiAssetsChecked, summary.m_uiErrors, summary.m_uiWarnings, summary.m_uiFixed, summary.m_uiSaved);
  }

  if (summary.m_bCanceled)
    sStatus.Append(" (Check was canceled; results are partial.)");

  StatusLabel->setText(QString::fromUtf8(sStatus.GetData()));
}

void ezQtAssetCheckPanel::ResultTreeItemDoubleClicked(QTreeWidgetItem* pItem, int iColumn)
{
  if (pItem == nullptr)
    return;

  // Opens the asset and, if the link contains an object GUID, selects that object.
  const QString sLinkTarget = pItem->data(0, Qt::UserRole).toString();
  if (sLinkTarget.isEmpty())
    return;

  ezQtUiServices::GotoLinkTarget(sLinkTarget.toUtf8().data());
}
