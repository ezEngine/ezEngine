#include <PCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QSettings>
#include <QTimer>

ezQtAssetBrowserWidget::ezQtAssetBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
  m_uiKnownAssetFolderCount = 0;
  m_bDialogMode = false;

  setupUi(this);

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);

  m_pFilter = new ezQtAssetBrowserFilter(this);
  m_pModel = new ezQtAssetBrowserModel(this, m_pFilter);

  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // Tool Bar
  {
    m_pToolbar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("AssetBrowserToolBar");
    ToolBarLayout->insertWidget(0, m_pToolbar);
  }

  TreeFolderFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
                    SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this,
                    SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr,
            "signal/slot connection failed");
  connect(SearchWidget, &ezQtSearchWidget::textChanged, this, &ezQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::ProjectEventHandler, this));
}

ezQtAssetBrowserWidget::~ezQtAssetBrowserWidget()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::ProjectEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::AssetCuratorEventHandler, this));

  ListAssets->setModel(nullptr);
}

void ezQtAssetBrowserWidget::UpdateAssetTypes()
{
  ezSet<ezString> KnownAssetTypes;

  for (auto docman : ezDocumentManager::GetAllDocumentManagers())
  {
    if (!docman->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
      continue;

    const ezAssetDocumentManager* pAssetDocMan = static_cast<const ezAssetDocumentManager*>(docman);

    pAssetDocMan->QuerySupportedAssetTypes(KnownAssetTypes);
  }

  {
    ezQtScopedBlockSignals block(ListTypeFilter);

    ListTypeFilter->clear();

    ezStringBuilder sIconName;

    // 'All' Filter
    {
      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QLatin1String(":/AssetIcons/All")), QLatin1String("<All>"));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Checked);

      ListTypeFilter->addItem(pItem);
    }

    for (const auto& key : KnownAssetTypes)
    {
      sIconName.Set(":/AssetIcons/", key);
      sIconName.ReplaceAll(" ", "_");
      sIconName.ReplaceAll("(", "");
      sIconName.ReplaceAll(")", "");

      QListWidgetItem* pItem = new QListWidgetItem(ezQtUiServices::GetCachedIconResource(sIconName), QString::fromUtf8(key.GetData()));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Unchecked);

      ListTypeFilter->addItem(pItem);
    }
  }

  UpdateDirectoryTree();

  // make sure to apply the previously active type filter settings to the UI
  OnTypeFilterChanged();
}

void ezQtAssetBrowserWidget::SetDialogMode()
{
  m_pToolbar->hide();
  m_bDialogMode = true;

  ListAssets->SetDialogMode(true);
}

void ezQtAssetBrowserWidget::SaveState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    Settings.setValue("SplitterGeometry", splitter->saveGeometry());
    Settings.setValue("SplitterState", splitter->saveState());
    Settings.setValue("IconSize", IconSizeSlider->value());
    Settings.setValue("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode);
  }
  Settings.endGroup();
}

void ezQtAssetBrowserWidget::RestoreState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    splitter->restoreGeometry(Settings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());
    splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
    IconSizeSlider->setValue(Settings.value("IconSize", IconSizeSlider->value()).toInt());

    if (Settings.value("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode).toBool())
      on_ButtonIconMode_clicked();
    else
      on_ButtonListMode_clicked();
  }
  Settings.endGroup();
}

void ezQtAssetBrowserWidget::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
    }
    break;
    case ezToolsProjectEvent::Type::ProjectClosed:
    {
      // remove project structure from asset browser
      ClearDirectoryTree();

      m_pFilter->Reset();
    }
    break;
  }
}


void ezQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_bDialogMode)
    return;

  const ezHybridArray<ezDocumentManager*, 16>& managers = ezDocumentManager::GetAllDocumentManagers();

  ezDynamicArray<const ezDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu("New");

  ezStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (ezDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const ezDocumentTypeDescriptor* a, const ezDocumentTypeDescriptor* b) -> bool {
    return a->m_sDocumentTypeName.Compare_NoCase(b->m_sDocumentTypeName) < 0;
  });

  for (const ezDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate)
      continue;
    if (desc->m_sFileExtension.IsEmpty())
      continue;
    // if (!sTypeFilter.IsEmpty() && !sTypeFilter.FindSubString(desc->m_sDocumentTypeName))
    // continue;

    QAction* pAction = pSubMenu->addAction(desc->m_sDocumentTypeName.GetData());
    pAction->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", qVariantFromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &ezQtAssetBrowserWidget::OnNewAsset);
  }
}

void ezQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  emit ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(),
                    m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                    m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void ezQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  emit ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(),
                    m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                    m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void ezQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();

  if (guid.IsValid())
  {
    ezAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
  }

  emit ItemChosen(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                  m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void ezQtAssetBrowserWidget::on_ButtonListMode_clicked()
{
  m_pModel->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void ezQtAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  m_pModel->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void ezQtAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void ezQtAssetBrowserWidget::on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage)
{
  ezQtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void ezQtAssetBrowserWidget::OnTextFilterChanged()
{
  SearchWidget->setText(QString::fromUtf8(m_pFilter->GetTextFilter()));

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void ezQtAssetBrowserWidget::OnTypeFilterChanged()
{
  ezStringBuilder sTemp;
  const ezStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");

  bool bAnyChecked = false;

  for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    sTemp.Set(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");

    const bool bChecked = sFilter.FindSubString(sTemp) != nullptr;

    ListTypeFilter->item(i)->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);

    if (bChecked)
      bAnyChecked = true;
  }

  ListTypeFilter->item(0)->setCheckState(bAnyChecked ? Qt::Unchecked : Qt::Checked);

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}


void ezQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void ezQtAssetBrowserWidget::on_ListTypeFilter_itemChanged(QListWidgetItem* item)
{
  ezQtScopedBlockSignals block(ListTypeFilter);

  if (item->text() == "<All>")
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate all others
      for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
      }
    }
    else
    {
      ezStringBuilder sFilter;

      // activate all others
      for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (!m_sAllTypesFilter.IsEmpty())
        {
          sFilter.Set(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");

          if (m_sAllTypesFilter.FindSubString(sFilter) != nullptr)
            ListTypeFilter->item(i)->setCheckState(Qt::Checked);
          else
            ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
        }
        else
          ListTypeFilter->item(i)->setCheckState(Qt::Checked);
      }
    }
  }
  else
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate the 'all' button
      ListTypeFilter->item(0)->setCheckState(Qt::Unchecked);
    }
    else
    {
      bool bAnyChecked = false;

      for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
          bAnyChecked = true;
      }

      // activate the 'All' item
      if (!bAnyChecked)
        ListTypeFilter->item(0)->setCheckState(Qt::Checked);
    }
  }

  ezStringBuilder sFilter;

  for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
      sFilter.Append(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");
  }

  if (sFilter.IsEmpty())         // all filters enabled
    sFilter = m_sAllTypesFilter; // might be different for dialogs

  m_pFilter->SetTypeFilter(sFilter);
}

void ezQtAssetBrowserWidget::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case ezAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    case ezAssetCuratorEvent::Type::AssetAdded:
    case ezAssetCuratorEvent::Type::AssetRemoved:
      UpdateDirectoryTree();
      break;
  }
}

void ezQtAssetBrowserWidget::UpdateDirectoryTree()
{
  ezQtScopedBlockSignals block(TreeFolderFilter);

  if (TreeFolderFilter->topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    TreeFolderFilter->addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);
  }

  const ezSet<ezString>& Folders = ezAssetCurator::GetSingleton()->GetAllAssetFolders();

  if (m_uiKnownAssetFolderCount == Folders.GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders.GetCount();

  ezStringBuilder tmp;

  for (const auto& sDir : Folders)
  {
    tmp = sDir;

    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
      continue;

    BuildDirectoryTree(tmp, TreeFolderFilter->topLevelItem(0), "", false);
  }

  TreeFolderFilter->setSortingEnabled(true);
  TreeFolderFilter->sortItems(0, Qt::SortOrder::AscendingOrder);
}


void ezQtAssetBrowserWidget::ClearDirectoryTree()
{
  TreeFolderFilter->clear();
  m_uiKnownAssetFolderCount = 0;
}

void ezQtAssetBrowserWidget::BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem,
                                                bool bIsHidden)
{
  if (ezStringUtils::IsNullOrEmpty(szCurPath))
    return;

  const char* szNextSep = ezStringUtils::FindSubString(szCurPath, "/");

  QTreeWidgetItem* pNewParent = nullptr;

  ezString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = szCurPath;
  else
    sFolderName = ezStringView(szCurPath, szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  ezStringBuilder sCurPath = szCurPathToItem;
  sCurPath.AppendPath(sFolderName);

  const QString sQtFolderName = QString::fromUtf8(sFolderName.GetData());

  for (ezInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  pNewParent = new QTreeWidgetItem();
  pNewParent->setText(0, sQtFolderName);
  pNewParent->setData(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath, QString::fromUtf8(sCurPath.GetData()));

  if (bIsHidden)
  {
    pNewParent->setTextColor(0, qRgb(110, 110, 120));
  }

  pParent->addChild(pNewParent);

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(szNextSep + 1, pNewParent, sCurPath, bIsHidden);
}

void ezQtAssetBrowserWidget::on_TreeFolderFilter_itemSelectionChanged()
{
  ezStringBuilder sCurPath;

  if (!TreeFolderFilter->selectedItems().isEmpty())
  {
    sCurPath = TreeFolderFilter->selectedItems()[0]->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void ezQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  if (TreeFolderFilter->currentItem())
  {
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this,
                SLOT(OnTreeOpenExplorer()));
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in sub-folders"), this, SLOT(OnShowSubFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInSubFolders());
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
  }

  AddAssetCreatorMenu(&m, false);

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void ezQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());
}

void ezQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void ezQtAssetBrowserWidget::OnTreeOpenExplorer()
{
  if (!TreeFolderFilter->currentItem())
    return;

  ezStringBuilder sPath =
      TreeFolderFilter->currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  if (!ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
    return;

  ezQtUiServices::OpenInExplorer(sPath, false);
}

void ezQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  if (ListAssets->selectionModel()->hasSelection())
  {
    if (!m_bDialogMode)
      m.setDefaultAction(m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Document"), this,
                                     SLOT(OnListOpenAssetDocument())));
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/AssetNeedsTransform16.png")), QLatin1String("Transform"), this,
                SLOT(OnTransform()));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this,
                SLOT(OnListOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentGuid16.png")), QLatin1String("Copy Asset Guid"), this,
                SLOT(OnListCopyAssetGuid()));
  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

  AddAssetCreatorMenu(&m, true);

  m.exec(ListAssets->viewport()->mapToGlobal(pt));
}

void ezQtAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  for (auto& index : selection)
  {
    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();

    if (guid.IsValid())
    {
      ezAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }

    emit ItemChosen(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                    m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void ezQtAssetBrowserWidget::OnTransform()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  ezProgressRange range("Transforming Assets", 1 + selection.length(), true);

  for (auto& index : selection)
  {
    if (range.WasCanceled())
      break;

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AssetGuid).value<ezUuid>();
    QString sPath = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString();
    range.BeginNextStep(sPath.toUtf8());
    auto res = ezAssetCurator::GetSingleton()->TransformAsset(guid, false);
    if (res.m_Result.Failed())
    {
      ezLog::Error("{0} ({1})", res.m_sMessage, sPath.toUtf8().data());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  ezAssetCurator::GetSingleton()->WriteAssetTables();
}

void ezQtAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  ezString sPath = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  ezQtUiServices::OpenInExplorer(sPath, true);
}

void ezQtAssetBrowserWidget::OnListCopyAssetGuid()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  ezStringBuilder tmp;
  ezUuid guid = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(ezConversionUtils::ToString(guid, tmp).GetData());
  clipboard->setMimeData(mimeData);
}


void ezQtAssetBrowserWidget::OnSelectionTimer()
{
  if (m_pModel->rowCount() == 1)
  {
    auto index = m_pModel->index(0, 0);

    ListAssets->selectionModel()->select(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }
}


void ezQtAssetBrowserWidget::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    emit ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    emit ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                      m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}

void ezQtAssetBrowserWidget::OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    emit ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    emit ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(),
                      m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void ezQtAssetBrowserWidget::OnModelReset()
{
  emit ItemCleared();
}


void ezQtAssetBrowserWidget::OnNewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  ezAssetDocumentManager* pManager = (ezAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  ezString sAssetType = pSender->property("AssetType").toString().toUtf8().data();
  ezString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir = ezToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  // find path
  {
    if (TreeFolderFilter->currentItem())
    {
      ezString sPath = TreeFolderFilter->currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty() && ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
        sStartDir = sPath.GetData();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      ezString sPath =
          m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty() && ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        ezStringBuilder temp = sPath;
        sPath = temp.GetFileDirectory();

        sStartDir = sPath.GetData();
      }
    }
  }

  ezStringBuilder title("Create ", sAssetType), sFilter;

  sFilter.Format("{0} (*.{1})", sAssetType, sExtension);

  QString sSelectedFilter = sFilter.GetData();
  ezStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), title.GetData(), sStartDir, sFilter.GetData(),
                                                         &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks)
                                .toUtf8()
                                .data();

  if (sOutput.IsEmpty())
    return;

  ezDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sOutput, pDoc, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList).m_Result.Succeeded())
  {
    pDoc->EnsureVisible();
  }
}

void ezQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void ezQtAssetBrowserWidget::OnPathFilterChanged()
{
  const QString sPath = QString::fromUtf8(m_pFilter->GetPathFilter());

  if (TreeFolderFilter->topLevelItemCount() == 1)
    SelectPathFilter(TreeFolderFilter->topLevelItem(0), sPath);

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

bool ezQtAssetBrowserWidget::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString() == sPath)
  {
    pParent->setSelected(true);
    return true;
  }

  for (ezInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (SelectPathFilter(pParent->child(i), sPath))
    {
      pParent->setExpanded(true);
      return true;
    }
  }

  return false;
}

void ezQtAssetBrowserWidget::SetSelectedAsset(ezUuid preselectedAsset)
{
  if (!preselectedAsset.IsValid())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(ezUuid, preselectedAsset));
}

void ezQtAssetBrowserWidget::OnScrollToItem(ezUuid preselectedAsset)
{
  for (ezInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    if (m_pModel->data(m_pModel->index(i, 0), ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(m_pModel->index(i, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(m_pModel->index(i, 0), QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void ezQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(const char* szFilters)
{
  m_sAllTypesFilter.Clear();

  if (!ezStringUtils::IsNullOrEmpty(szFilters))
  {
    ezQtScopedBlockSignals block(ListTypeFilter);

    ezStringBuilder sFilter;
    const ezStringBuilder sAllFilters(";", szFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
    {
      sFilter.Set(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");

      if (sAllFilters.FindSubString(sFilter) == nullptr)
        ListTypeFilter->item(i)->setHidden(true);
      // ListTypeFilter->item(i)->setFlags(Qt::ItemFlag::ItemIsUserCheckable);
    }
  }

  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
}
