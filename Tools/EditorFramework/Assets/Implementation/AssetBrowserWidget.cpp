#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Basics.h>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QSettings>
#include <QMenu>

ezAssetBrowserWidget::ezAssetBrowserWidget(QWidget* parent) : QWidget(parent)
{
  m_uiKnownAssetFolderCount = 0;
  m_bDialogMode = false;

  setupUi(this);

  m_pModel = new ezAssetBrowserModel(this);

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // Tool Bar
  {
    m_pToolbar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("TextureAssetWindowToolBar");
    ToolBarLayout->insertWidget(0, m_pToolbar);
  }

  TreeFolderFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pModel, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");

  UpdateAssetTypes();

  ezAssetCurator::GetInstance()->m_Events.AddEventHandler(ezMakeDelegate(&ezAssetBrowserWidget::AssetCuratorEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezAssetBrowserWidget::ProjectEventHandler, this));
}

ezAssetBrowserWidget::~ezAssetBrowserWidget()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetBrowserWidget::ProjectEventHandler, this));
  ezAssetCurator::GetInstance()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetBrowserWidget::AssetCuratorEventHandler, this));

  ListAssets->setModel(nullptr);
}

void ezAssetBrowserWidget::UpdateAssetTypes()
{
  ezSet<ezString> KnownAssetTypes;

  for (auto docman : ezDocumentManagerBase::GetAllDocumentManagers())
  {
    if (!docman->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
      continue;

    const ezAssetDocumentManager* pAssetDocMan = static_cast<const ezAssetDocumentManager*>(docman);

    pAssetDocMan->QuerySupportedAssetTypes(KnownAssetTypes);
  }

  {
    QtScopedBlockSignals block(ListTypeFilter);

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

      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QString::fromUtf8(sIconName.GetData())), QString::fromUtf8(key.GetData()));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Unchecked);

      ListTypeFilter->addItem(pItem);
    }
  }

  UpdateDirectoryTree();

}

void ezAssetBrowserWidget::SetDialogMode()
{
  m_pToolbar->hide();
  m_bDialogMode = true;

  ListAssets->SetDialogMode(true);
}

void ezAssetBrowserWidget::SaveState(const char* szSettingsName)
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

void ezAssetBrowserWidget::RestoreState(const char* szSettingsName)
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

void ezAssetBrowserWidget::ProjectEventHandler(const ezToolsProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
    }
    break;
  }
}

void ezAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex & index)
{
  emit ItemSelected(m_pModel->data(index, Qt::UserRole + 0).toString(), m_pModel->data(index, Qt::UserRole + 2).toString(), m_pModel->data(index, Qt::UserRole + 1).toString());
}

void ezAssetBrowserWidget::on_ListAssets_activated(const QModelIndex & index)
{
  emit ItemSelected(m_pModel->data(index, Qt::UserRole + 0).toString(), m_pModel->data(index, Qt::UserRole + 2).toString(), m_pModel->data(index, Qt::UserRole + 1).toString());
}

void ezAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  QString sGuid = m_pModel->data(index, Qt::UserRole + 0).toString();

  if (!sGuid.isEmpty())
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid.toUtf8().data());
    ezAssetCurator::GetInstance()->UpdateAssetLastAccessTime(guid);
  }

  emit ItemChosen(sGuid, m_pModel->data(index, Qt::UserRole + 2).toString(), m_pModel->data(index, Qt::UserRole + 1).toString());
}

void ezAssetBrowserWidget::on_ButtonListMode_clicked()
{
  m_pModel->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void ezAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  m_pModel->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void ezAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void ezAssetBrowserWidget::on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage)
{
  QtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void ezAssetBrowserWidget::OnTextFilterChanged()
{
  LineSearchFilter->setText(QString::fromUtf8(m_pModel->GetTextFilter()));
}

void ezAssetBrowserWidget::OnTypeFilterChanged()
{
  ezStringBuilder sTemp;
  const ezStringBuilder sFilter(";", m_pModel->GetTypeFilter(), ";");

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
}

void ezAssetBrowserWidget::on_LineSearchFilter_textEdited(const QString& text)
{
  m_pModel->SetTextFilter(text.toUtf8().data());
}

void ezAssetBrowserWidget::on_ButtonClearSearch_clicked()
{
  m_pModel->SetTextFilter("");
  LineSearchFilter->setFocus();
}

void ezAssetBrowserWidget::on_ListTypeFilter_itemChanged(QListWidgetItem* item)
{
  QtScopedBlockSignals block(ListTypeFilter);

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

  if (sFilter.IsEmpty()) // all filters enabled
    sFilter = m_sAllTypesFilter; // might be different for dialogs

  m_pModel->SetTypeFilter(sFilter);
}

void ezAssetBrowserWidget::AssetCuratorEventHandler(const ezAssetCurator::Event& e)
{
  switch (e.m_Type)
  {
  case ezAssetCurator::Event::Type::AssetListReset:
    UpdateAssetTypes();
    break;
  case ezAssetCurator::Event::Type::AssetAdded:
  case ezAssetCurator::Event::Type::AssetRemoved:
    UpdateDirectoryTree();
    break;
  }
}

void ezAssetBrowserWidget::UpdateDirectoryTree()
{
  QtScopedBlockSignals block(TreeFolderFilter);

  if (TreeFolderFilter->topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    TreeFolderFilter->addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);
  }

  const ezSet<ezString>& Folders = ezAssetCurator::GetInstance()->GetAllAssetFolders();

  if (m_uiKnownAssetFolderCount == Folders.GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders.GetCount();

  for (auto sDir : Folders)
  {
    if (!ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(sDir))
      continue;

    BuildDirectoryTree(sDir, TreeFolderFilter->topLevelItem(0), "");
  }

  TreeFolderFilter->setSortingEnabled(true);
  TreeFolderFilter->sortItems(0, Qt::SortOrder::AscendingOrder);
}

void ezAssetBrowserWidget::BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem)
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
  pNewParent->setData(0, Qt::UserRole + 1, QString::fromUtf8(sCurPath.GetData()));

  pParent->addChild(pNewParent);

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(szNextSep + 1, pNewParent, sCurPath);
}

void ezAssetBrowserWidget::on_TreeFolderFilter_itemSelectionChanged()
{
  ezStringBuilder sCurPath;

  if (!TreeFolderFilter->selectedItems().isEmpty())
  {
    sCurPath = TreeFolderFilter->selectedItems()[0]->data(0, Qt::UserRole + 1).toString().toUtf8().data();
  }

  m_pModel->SetPathFilter(sCurPath);
}

void ezAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  if (TreeFolderFilter->currentItem())
  {
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this, SLOT(OnTreeOpenExplorer()));
  }

  QAction* pAction = m.addAction(QLatin1String("Show Items in Sub-Folders"), this, SLOT(OnShowSubFolderItemsToggled()));
  pAction->setCheckable(true);
  pAction->setChecked(m_pModel->GetShowItemsInSubFolders());

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void ezAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pModel->SetShowItemsInSubFolders(!m_pModel->GetShowItemsInSubFolders());
}

void ezAssetBrowserWidget::OnTreeOpenExplorer()
{
  if (!TreeFolderFilter->currentItem())
    return;

  ezString sPath = TreeFolderFilter->currentItem()->data(0, Qt::UserRole + 1).toString().toUtf8().data();

  if (!ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  ezUIServices::OpenInExplorer(sPath);
}

void ezAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  if (ListAssets->selectionModel()->hasSelection())
  {

    if (!m_bDialogMode)
      m.setDefaultAction(m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Document"), this, SLOT(OnListOpenAssetDocument())));
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open Containing Folder"), this, SLOT(OnListOpenExplorer()));
  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pModel->GetSortByRecentUse());

  m.exec(ListAssets->viewport()->mapToGlobal(pt));
}

void ezAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  auto index = ListAssets->currentIndex();

  QString sGuid = m_pModel->data(index, Qt::UserRole + 0).toString();

  if (!sGuid.isEmpty())
  {
    ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid.toUtf8().data());
    ezAssetCurator::GetInstance()->UpdateAssetLastAccessTime(guid);
  }

  emit ItemChosen(sGuid, m_pModel->data(index, Qt::UserRole + 2).toString(), m_pModel->data(index, Qt::UserRole + 1).toString());
}

void ezAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  ezString sPath = m_pModel->data(ListAssets->currentIndex(), Qt::UserRole + 1).toString().toUtf8().data();

  ezUIServices::OpenInExplorer(sPath);
}

void ezAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pModel->SetSortByRecentUse(!m_pModel->GetSortByRecentUse());
}

void ezAssetBrowserWidget::OnPathFilterChanged()
{
  const QString sPath = QString::fromUtf8(m_pModel->GetPathFilter());

  if (TreeFolderFilter->topLevelItemCount() == 1)
    SelectPathFilter(TreeFolderFilter->topLevelItem(0), sPath);
}

bool ezAssetBrowserWidget::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, Qt::UserRole + 1).toString() == sPath)
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

void ezAssetBrowserWidget::SetSelectedAsset(const char* szAssetPath)
{
  if (ezStringUtils::IsNullOrEmpty(szAssetPath))
    return;

  const QString sPath = QString::fromUtf8(szAssetPath);

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, sPath));
}

void ezAssetBrowserWidget::OnScrollToItem(QString sPath)
{
  for (ezInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    if (m_pModel->data(m_pModel->index(i, 0), Qt::UserRole + 2) == sPath)
    {
      ListAssets->selectionModel()->select(m_pModel->index(i, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(m_pModel->index(i, 0), QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }
}

void ezAssetBrowserWidget::ShowOnlyTheseTypeFilters(const char* szFilters)
{
  m_sAllTypesFilter.Clear();

  if (!ezStringUtils::IsNullOrEmpty(szFilters))
  {
    QtScopedBlockSignals block(ListTypeFilter);

    ezStringBuilder sFilter;
    const ezStringBuilder sAllFilters(";", szFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
    {
      sFilter.Set(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");

      if (sAllFilters.FindSubString(sFilter) == nullptr)
        ListTypeFilter->item(i)->setHidden(true);
        //ListTypeFilter->item(i)->setFlags(Qt::ItemFlag::ItemIsUserCheckable);
    }
  }

  m_pModel->SetTypeFilter(m_sAllTypesFilter);
}

