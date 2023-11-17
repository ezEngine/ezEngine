#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <QFile>

ezQtAssetBrowserWidget::ezQtAssetBrowserWidget(QWidget* pParent)
  : QWidget(pParent)
{
  m_bDialogMode = false;

  setupUi(this);

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);

  m_pFilter = new ezQtAssetBrowserFilter(this);
  TreeFolderFilter->SetFilter(m_pFilter);

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



  EZ_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, &ezQtAssetBrowserModel::editingFinished, this, &ezQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
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
  const auto& assetTypes0 = ezAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  ezMap<ezString, const ezDocumentTypeDescriptor*> assetTypes;
  for (auto it : assetTypes0)
  {
    assetTypes[ezTranslate(it.Key())] = it.Value();
  }

  {
    ezQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    if (!m_bDialogMode)
    {
      // '<All Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("<All Files>"));

      // '<Importable Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/EditorFramework/Icons/ImportableFileType.svg")), QLatin1String("<Importable Files>"));
    }

    // '<All Assets>' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/Icons/AllAssets.svg")), QLatin1String("<All Assets>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(ezQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, ezColorScheme::GetCategoryColor(it.Value()->m_sAssetCategory, ezColorScheme::CategoryColorUsage::AssetMenuIcon)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  // make sure to apply the previously active type filter settings to the UI
  ezSet<ezString> importExtensions;
  ezAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
  m_pFilter->UpdateImportExtensions(importExtensions);
  OnTypeFilterChanged();
}

void ezQtAssetBrowserWidget::SetDialogMode()
{
  if (m_bDialogMode)
    return;

  m_pToolbar->hide();
  m_bDialogMode = true;

  TreeFolderFilter->SetDialogMode(true);
  ListAssets->SetDialogMode(true);

  UpdateAssetTypes();
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
      m_pFilter->Reset();
    }
    break;
    default:
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

  documentTypes.Sort([](const ezDocumentTypeDescriptor* a, const ezDocumentTypeDescriptor* b) -> bool
    { return ezStringUtils::Compare(ezTranslate(a->m_sDocumentTypeName), ezTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(ezTranslate("Folder"));
  pAction->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const ezDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(ezTranslate(desc->m_sDocumentTypeName));
    pAction->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, ezColorScheme::GetCategoryColor(desc->m_sAssetCategory, ezColorScheme::CategoryColorUsage::MenuEntryIcon)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &ezQtAssetBrowserWidget::NewAsset);
  }
}


void ezQtAssetBrowserWidget::AddImportedViaMenu(QMenu* pMenu)
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  // Find all uses
  ezSet<ezUuid> importedVia;
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (!bImportable)
      continue;

    ezString sAbsPath = qtToEzString(m_pModel->data(id, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
    ezAssetCurator::GetSingleton()->FindAllUses(sAbsPath, importedVia);
  }

  // Sort by path
  ezHybridArray<ezUuid, 8> importedViaSorted;
  {
    importedViaSorted.Reserve(importedVia.GetCount());
    ezAssetCurator::ezLockedSubAssetTable allAssets = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
    for (const ezUuid& guid : importedVia)
    {
      if (allAssets->Contains(guid))
        importedViaSorted.PushBack(guid);
    }

    importedViaSorted.Sort([&](const ezUuid& a, const ezUuid& b) -> bool
      { return allAssets->Find(a).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath().Compare(allAssets->Find(b).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath()) < 0; });
  }

  if (importedViaSorted.IsEmpty())
    return;

  // Create actions to open
  QMenu* pSubMenu = pMenu->addMenu("Imported via");
  pSubMenu->setIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")));

  for (const ezUuid& guid : importedViaSorted)
  {
    const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
    QIcon icon = ezQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, ezColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, ezColorScheme::CategoryColorUsage::OverlayIcon));
    QString sRelPath = ezMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

    QAction* pAction = pSubMenu->addAction(sRelPath);
    pAction->setIcon(icon);
    pAction->setProperty("AbsPath", ezMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()));
    connect(pAction, &QAction::triggered, this, &ezQtAssetBrowserWidget::OnOpenImportReferenceAsset);
  }
}

void ezQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void ezQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void ezQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  const ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();

  if (itemType.IsAnySet(ezAssetBrowserItemFlags::Asset | ezAssetBrowserItemFlags::SubAsset))
  {
    if (guid.IsValid())
    {
      ezAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }
    Q_EMIT ItemChosen(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
  else if (itemType.IsSet(ezAssetBrowserItemFlags::File))
  {
    QString sAbsPath = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    ezQtUiServices::OpenFileInDefaultProgram(qtToEzString(sAbsPath));
  }
  else if (itemType.IsAnySet(ezAssetBrowserItemFlags::Folder | ezAssetBrowserItemFlags::DataDirectory))
  {
    m_pFilter->SetPathFilter(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data());
  }
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

  {
    ezQtScopedBlockSignals _(TypeFilter);

    ezInt32 iCheckedFilter = 0;
    ezInt32 iNumChecked = 0;

    for (ezInt32 i = 1; i < TypeFilter->count(); ++i)
    {
      sTemp.Set(";", TypeFilter->itemData(i, Qt::UserRole).toString().toUtf8().data(), ";");

      if (sFilter.FindSubString(sTemp) != nullptr)
      {
        ++iNumChecked;
        iCheckedFilter = i;
      }
    }

    if (iNumChecked == 1)
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex(m_bDialogMode ? 0 : 2); // "<All Assets>"

    int index = TypeFilter->currentIndex();
    m_pFilter->SetShowNonImportableFiles(!m_bDialogMode && index == 0);
    m_pFilter->SetShowFiles(!m_bDialogMode && (index == 0 || index == 1));
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void ezQtAssetBrowserWidget::OnPathFilterChanged()
{
  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void ezQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void ezQtAssetBrowserWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && !m_bDialogMode)
  {
    e->accept();
    DeleteSelection();
    return;
  }
}


void ezQtAssetBrowserWidget::DeleteSelection()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)id.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (itemType.IsAnySet(ezAssetBrowserItemFlags::SubAsset | ezAssetBrowserItemFlags::DataDirectory))
    {
      ezQtUiServices::MessageBoxWarning(ezFmt("Sub-assets and data directories can't be deleted."));
      return;
    }
  }

  QMessageBox::StandardButton choice = ezQtUiServices::MessageBoxQuestion(ezFmt("Do you want to delete the selected items?"), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
  if (choice == QMessageBox::StandardButton::Cancel)
    return;

  for (const QModelIndex& id : selection)
  {
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)id.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    QString sQtAbsPath = id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    ezString sAbsPath = qtToEzString(sQtAbsPath);

    if (itemType.IsSet(ezAssetBrowserItemFlags::File))
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        ezLog::Error("Failed to delete file '{}'", sAbsPath);
      }
    }
    else
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        ezLog::Error("Failed to delete folder '{}'", sAbsPath);
      }
    }
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void ezQtAssetBrowserWidget::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case ezAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    default:
      break;
  }
}

void ezQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  const bool bClickedValid = TreeFolderFilter->indexAt(pt).isValid();

  if (bClickedValid)
  {
    const bool bIsRoot = TreeFolderFilter->currentItem() && TreeFolderFilter->currentItem() == TreeFolderFilter->topLevelItem(0);
    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), TreeFolderFilter, SLOT(TreeOpenExplorer()));
    }

    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      // Delete
      const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)TreeFolderFilter->currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), TreeFolderFilter, &eqQtAssetBrowserFolderView::DeleteFolder);
      if (itemType.IsSet(ezAssetBrowserItemFlags::DataDirectory))
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Data directories can't be deleted.");
      }

      // Create
      AddAssetCreatorMenu(&m, false);
    }

    m.addSeparator();
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
    pAction->setEnabled(m_pFilter->GetShowItemsInSubFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void ezQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  ezQtScopedBlockSignals block(TypeFilter);

  ezStringBuilder sFilter;

  m_pFilter->SetShowNonImportableFiles(!m_bDialogMode && index == 0);
  m_pFilter->SetShowFiles(!m_bDialogMode && (index == 0 || index == 1));

  switch (index)
  {
    case 0:
    case 1:
    case 2:
      // all filters enabled
      // might be different for dialogs
      sFilter = m_sAllTypesFilter;
      break;

    default:
      sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
      break;
  }

  m_pFilter->SetTypeFilter(sFilter);
}

void ezQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());
}

void ezQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void ezQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    if (!m_bDialogMode)
    {
      QString sTitle = "Open Selection";
      QIcon icon = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));
      if (ListAssets->selectionModel()->selectedIndexes().count() == 1)
      {
        const QModelIndex firstItem = ListAssets->selectionModel()->selectedIndexes()[0];
        const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)firstItem.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(ezAssetBrowserItemFlags::Asset | ezAssetBrowserItemFlags::SubAsset))
        {
          sTitle = "Open Document";
        }
        else if (itemType.IsSet(ezAssetBrowserItemFlags::File))
        {
          sTitle = "Open File";
        }
        else if (itemType.IsAnySet(ezAssetBrowserItemFlags::DataDirectory | ezAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/AssetNeedsTransform.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]()
      { OnListFindAllReferences(false); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]()
      { OnListFindAllReferences(true); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());


  if (!m_bDialogMode && ListAssets->selectionModel()->hasSelection())
  {
    QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
    bool bImportable = false;
    bool bAllFiles = true;
    for (const QModelIndex& id : selection)
    {
      bImportable |= id.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();

      const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)id.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(ezAssetBrowserItemFlags::SubAsset | ezAssetBrowserItemFlags::DataDirectory))
      {
        bAllFiles = false;
      }
    }

    // Delete
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(DeleteSelection()));
      if (!bAllFiles)
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Sub-assets and data directories can't be deleted.");
      }
    }

    // Import assets
    if (bImportable)
    {
      m.addSeparator();
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), QLatin1String("Import..."), this, SLOT(ImportSelection()));
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();
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
    // Only enter folders on a single selection. Otherwise the results are undefined.
    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (selection.count() > 1 && itemType.IsAnySet(ezAssetBrowserItemFlags::DataDirectory | ezAssetBrowserItemFlags::Folder))
      continue;
    on_ListAssets_doubleClicked(index);
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
    ezTransformStatus res = ezAssetCurator::GetSingleton()->TransformAsset(guid, ezTransformFlags::TriggeredManually);
    if (res.Failed())
    {
      ezLog::Error("{0} ({1})", res.m_sMessage, sPath.toUtf8().data());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
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

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Copied asset GUID: {}", tmp), ezTime::MakeFromSeconds(5));
}

void ezQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  ezStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  m_pFilter->SetPathFilter(sPath);
}

void ezQtAssetBrowserWidget::OnListFindAllReferences(bool transitive)
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  ezUuid guid = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
  ezStringBuilder sAssetGuid;
  ezConversionUtils::ToString(guid, sAssetGuid);

  ezStringBuilder sFilter;
  sFilter.Format("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
  m_pFilter->SetTextFilter(sFilter);
  m_pFilter->SetPathFilter("");
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
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}

void ezQtAssetBrowserWidget::OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void ezQtAssetBrowserWidget::OnModelReset()
{
  Q_EMIT ItemCleared();
}


void ezQtAssetBrowserWidget::NewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  ezAssetDocumentManager* pManager = (ezAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  ezString sAssetType = pSender->property("AssetType").toString().toUtf8().data();
  ezString sTranslateAssetType = ezTranslate(sAssetType);
  ezString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir = ezToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  // find path
  {
    if (TreeFolderFilter->currentItem())
    {
      sStartDir = TreeFolderFilter->currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      ezString sPath = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty())
      {
        ezStringBuilder temp = sPath;
        sPath = temp.GetFileDirectory();

        sStartDir = sPath.GetData();
      }
    }
  }

  //
  ezStringBuilder sNewAsset = qtToEzString(sStartDir);
  ezStringBuilder sBaseFileName;
  ezPathUtils::MakeValidFilename(sTranslateAssetType, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (ezUInt32 i = 0; ezOSFile::ExistsFile(sNewAsset); i++)
  {
    sNewAsset = qtToEzString(sStartDir);
    sNewAsset.AppendFormat("/{}{}.{}", sBaseFileName, i, sExtension);
  }

  sNewAsset.MakeCleanPath();

  ezDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sNewAsset, pDoc, ezDocumentFlags::Default).m_Result.Failed())
  {
    ezLog::Error("Failed to create document: {}", sNewAsset);
    return;
  }

  {
    ezStringBuilder sRelativePath = sNewAsset;
    if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelativePath))
    {
      m_pFilter->SetTemporaryPinnedItem(sRelativePath);
    }
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sNewAsset);
    m_pModel->OnFileSystemUpdate();
  }

  ezInt32 iNewIndex = m_pModel->FindIndex(sNewAsset);
  if (iNewIndex != -1)
  {
    m_bOpenAfterRename = true;
    QModelIndex idx = m_pModel->index(iNewIndex, 0);
    ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    ListAssets->edit(idx);
  }
}


void ezQtAssetBrowserWidget::OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset)
{
  ezStringBuilder sOldPath = qtToEzString(sAbsPath);
  ezStringBuilder sNewPath = sOldPath;
  if (bIsAsset)
    sNewPath.ChangeFileName(qtToEzString(sNewName));
  else
    sNewPath.ChangeFileNameAndExtension(qtToEzString(sNewName));

  if (sOldPath != sNewPath)
  {
    if (ezOSFile::MoveFileOrDirectory(sOldPath, sNewPath).Failed())
    {
      ezLog::Error("Failed to rename '{}' to '{}'", sOldPath, sNewPath);
      return;
    }

    ezFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sOldPath);

    // If we have a temporary item, make sure that any renames ensure that the item is still set as the new temporary
    // A common case is: a type filter is active that excludes a newly created asset. Thus, on creation the new asset is set as the pinned item. Editing of the item is started and the user gives it a new name and we end up here. We want the item to remain pinned.
    if (!m_pFilter->GetTemporaryPinnedItem().IsEmpty())
    {
      ezStringBuilder sOldRelativePath = sOldPath;
      ezStringBuilder sNewRelativePath = sNewPath;
      if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sOldRelativePath) && ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewRelativePath))
      {
        if (sOldRelativePath == m_pFilter->GetTemporaryPinnedItem())
        {
          m_pFilter->SetTemporaryPinnedItem(sNewRelativePath);
        }
      }
    }
    m_pModel->OnFileSystemUpdate();

    ezInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    }
  }

  if (m_bOpenAfterRename)
  {
    m_bOpenAfterRename = false;

    // I don't understand why, but if we immediately execute on_ListAssets_doubleClicked() here, then the opened document will be in a broken state
    // maybe something else has to happen on the main thread first ?
    // using a delay (even of 0) fixes the problem
    QTimer::singleShot(100, this, [this, sNewPath]()
      {
        ezInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
        if (iNewIndex != -1)
        {
          QModelIndex idx = m_pModel->index(iNewIndex, 0);
          on_ListAssets_doubleClicked(idx);
        } //
      });
  }
}


void ezQtAssetBrowserWidget::ImportSelection()
{
  ezHybridArray<ezString, 4> filesToImport;
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      filesToImport.PushBack(qtToEzString(id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
  if (filesToImport.IsEmpty())
    return;

  ezAssetDocumentGenerator::ImportAssets(filesToImport);

  for (const QModelIndex& id : selection)
  {
    Q_EMIT m_pModel->dataChanged(id, id);
  }
}


void ezQtAssetBrowserWidget::OnOpenImportReferenceAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());
  ezString sAbsPath = qtToEzString(pSender->property("AbsPath").toString());

  ezQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList);
}

void ezQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
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
    QModelIndex idx = m_pModel->index(i, 0);
    if (m_pModel->data(idx, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
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
    ezStringBuilder sFilter;
    const ezStringBuilder sAllFilters(";", szFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    {
      ezQtScopedBlockSignals block(TypeFilter);

      for (ezInt32 i = TypeFilter->count(); i > 1; --i)
      {
        const ezInt32 idx = i - 1;

        sFilter.Set(";", TypeFilter->itemData(idx, Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          TypeFilter->removeItem(idx);
        }
      }
    }
  }

  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
}
