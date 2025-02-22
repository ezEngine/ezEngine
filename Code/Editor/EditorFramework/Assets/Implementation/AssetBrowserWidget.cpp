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

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QFile>

ezQtAssetBrowserWidget::ezQtAssetBrowserWidget(QWidget* pParent)
  : QWidget(pParent)
{
  setupUi(this);

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);
  ResetTypeFilter->setEnabled(false);

  m_pFilter = new ezQtAssetBrowserFilter(this);
  m_pFilter->SetShowItemsInSubFolders(pPreferences->m_bAssetBrowserShowItemsInSubFolders);

  TreeFolderFilter->SetFilter(m_pFilter);

  m_pModel = new ezQtAssetBrowserModel(this, m_pFilter);
  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  ListAssets->setDragEnabled(true);
  ListAssets->setAcceptDrops(true);
  ListAssets->setDropIndicatorShown(true);
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

  ButtonShowItemsSubFolders->setEnabled(true);
  ButtonShowItemsSubFolders->setChecked(m_pFilter->GetShowItemsInSubFolders());
  EZ_VERIFY(connect(ButtonShowItemsSubFolders, SIGNAL(toggled(bool)), this, SLOT(OnShowSubFolderItemsToggled())) != nullptr, "signal/slot connection failed");

  EZ_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pFilter, SIGNAL(FilterChanged()), this, SLOT(OnFilterChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pModel, &ezQtAssetBrowserModel::editingFinished, this, &ezQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  connect(SearchWidget, &ezQtSearchWidget::textChanged, this, &ezQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::ProjectEventHandler, this));

  setAcceptDrops(true);
}

ezQtAssetBrowserWidget::~ezQtAssetBrowserWidget()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::ProjectEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtAssetBrowserWidget::AssetCuratorEventHandler, this));


  ListAssets->setModel(nullptr);
}

void ezQtAssetBrowserWidget::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (!pEvent->source())
    pEvent->acceptProposedAction();
}

void ezQtAssetBrowserWidget::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void ezQtAssetBrowserWidget::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

static void CleanUpFiles(ezArrayPtr<ezString> files)
{
  for (const auto& file : files)
  {
    ezOSFile::DeleteFile(file).IgnoreResult();
    ezFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }
}

void ezQtAssetBrowserWidget::dropEvent(QDropEvent* pEvent)
{
  const QMimeData* mime = pEvent->mimeData();
  if (!mime->hasUrls())
  {
    pEvent->ignore();
  }

  pEvent->acceptProposedAction();

  ezStringBuilder sTargetDir;

  if (TreeFolderFilter->currentItem() != nullptr)
  {
    sTargetDir = TreeFolderFilter->currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  }

  if (sTargetDir.IsEmpty())
  {
    ezQtUiServices::MessageBoxInformation("Please first select a folder in the asset browser as the destination for the file import.");
    return;
  }

  QList<QUrl> urlList = mime->urls();
  ezHybridArray<ezString, 16> assetsToImport;

  // if we leave this function prematurely, delete all these temp files
  EZ_SCOPE_EXIT(CleanUpFiles(assetsToImport));

  bool overWriteAll = false;
  for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
  {
    QUrl url = urlList.at(i);
    QFileInfo fileinfo(url.toLocalFile());

    if (!fileinfo.exists())
      continue;

    // build source and destination paths info
    ezStringBuilder srcPath = urlList.at(i).path().toUtf8().constData();
    srcPath.TrimWordStart("/"); // remove the "/" at the beginning of the source path

    ezStringBuilder dstPath = sTargetDir;
    dstPath.AppendPath(fileinfo.fileName().toUtf8().constData());

    // Move the file/folder
    if (fileinfo.isDir())
    {
      if (!overWriteAll && ezOSFile::ExistsDirectory(dstPath))
      {
        const auto res = ezQtUiServices::MessageBoxQuestion(ezFmt("This folder already exists:\n'{}'\n\nOverwrite existing files inside it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel);

        switch (res)
        {
          case QMessageBox::Yes:
            break;

          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;

          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (ezOSFile::CopyFolder(srcPath, dstPath, &assetsToImport) != EZ_SUCCESS)
      {
        ezQtUiServices::MessageBoxWarning(ezFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }
    }
    else if (fileinfo.isFile())
    {
      if (!overWriteAll && ezOSFile::ExistsFile(dstPath))
      {
        const auto res = ezQtUiServices::MessageBoxQuestion(ezFmt("This file already exists:\n'{}'\n\nOverwrite it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel);

        switch (res)
        {
          case QMessageBox::Yes:
            break;
          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;
          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (ezOSFile::CopyFile(srcPath, dstPath) != EZ_SUCCESS)
      {
        ezQtUiServices::MessageBoxWarning(ezFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }

      assetsToImport.PushBack(dstPath);
    }
  }

  for (const auto& file : assetsToImport)
  {
    ezFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }

  QTimer::singleShot(1, this, [=]()
    {
      // return to the OS and import with a slight delay, otherwise the drop operation blocks the OS
      ezAssetDocumentGenerator::ImportAssets(assetsToImport);
      //
    });

  // now that we've successfully imported the assets, clear this list so that the files don't get deleted
  assetsToImport.Clear();
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

    if (m_Mode == Mode::Browser)
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
  if (m_Mode == Mode::Browser)
  {
    ezSet<ezString> importExtensions;
    ezAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
    m_pFilter->UpdateImportExtensions(importExtensions);
  }

  OnTypeFilterChanged();
}

void ezQtAssetBrowserWidget::SetMode(Mode mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pToolbar->show();
      TreeFolderFilter->SetDialogMode(false);
      ListAssets->SetDialogMode(false);
      break;
    case Mode::FilePicker:
      TypeFilter->setVisible(false);
      ResetTypeFilter->setVisible(false);
      [[fallthrough]];
    case Mode::AssetPicker:
      m_pToolbar->hide();
      TreeFolderFilter->SetDialogMode(true);
      ListAssets->SetDialogMode(true);
      break;
  }

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
  if (m_Mode != Mode::Browser)
    return;

  const ezHybridArray<ezDocumentManager*, 16>& managers = ezDocumentManager::GetAllDocumentManagers();

  ezDynamicArray<const ezDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu(QIcon(":/GuiFoundation/Icons/DocumentAdd.svg"), "New");

  ezStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (ezDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const ezDocumentTypeDescriptor* a, const ezDocumentTypeDescriptor* b) -> bool
    { return ezTranslate(a->m_sDocumentTypeName).Compare(ezTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(ezMakeQString(ezTranslate("Folder")));
  pAction->setIcon(ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const ezDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(ezMakeQString(ezTranslate(desc->m_sDocumentTypeName)));
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

void ezQtAssetBrowserWidget::GetSelectedImportableFiles(ezDynamicArray<ezString>& out_Files) const
{
  out_Files.Clear();

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(ezQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      out_Files.PushBack(qtToEzString(id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
}

void ezQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void ezQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
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
    Q_EMIT ItemChosen(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsSet(ezAssetBrowserItemFlags::File))
  {
    Q_EMIT ItemChosen(ezUuid::MakeInvalid(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
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

void ezQtAssetBrowserWidget::on_ResetTypeFilter_clicked()
{
  switch (m_Mode)
  {
    case Mode::Browser:
      TypeFilter->setCurrentIndex(2);
      break;
    case Mode::AssetPicker:
    case Mode::FilePicker:
      TypeFilter->setCurrentIndex(0);
      break;
  }
}

void ezQtAssetBrowserWidget::OnTextFilterChanged()
{
  OnFilterChanged();

  const QString sText = ezMakeQString(m_pFilter->GetTextFilter());
  if (SearchWidget->text() != sText)
  {
    SearchWidget->setText(sText);
    QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
  }
}

void ezQtAssetBrowserWidget::OnFilterChanged()
{
  const QString sText = ezMakeQString(m_pFilter->GetTextFilter());
  ButtonShowItemsSubFolders->setEnabled(sText.isEmpty());
  ButtonShowItemsSubFolders->blockSignals(true);
  ButtonShowItemsSubFolders->setChecked(!sText.isEmpty() || m_pFilter->GetShowItemsInSubFolders());
  ButtonShowItemsSubFolders->blockSignals(false);
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

    if (iNumChecked == ((m_Mode != Mode::Browser) ? 1 : 3))
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex((m_Mode != Mode::Browser) ? 0 : 2); // "<All Assets>"

    int index = TypeFilter->currentIndex();

    switch (m_Mode)
    {
      case Mode::Browser:
        m_pFilter->SetShowNonImportableFiles(index == 0);
        m_pFilter->SetShowFiles(index == 0 || index == 1);
        break;
      case Mode::AssetPicker:
        m_pFilter->SetShowNonImportableFiles(false);
        m_pFilter->SetShowFiles(false);
        break;
      case Mode::FilePicker:
        m_pFilter->SetShowNonImportableFiles(true);
        m_pFilter->SetShowFiles(true);
        break;
    }
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

  if (e->key() == Qt::Key_Delete && m_Mode == Mode::Browser)
  {
    e->accept();
    DeleteSelection();
    return;
  }

  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
  {
    e->accept();
    OnListOpenAssetDocument();
    return;
  }
}

void ezQtAssetBrowserWidget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->accept();
    ezStringBuilder sPath = m_pFilter->GetPathFilter();
    if (sPath.IsEmpty())
      return;
    sPath.PathParentDirectory();
    sPath.Trim("/");

    m_pFilter->SetPathFilter(sPath);
    return;
  }

  QWidget::mousePressEvent(e);
}

void ezQtAssetBrowserWidget::RenameCurrent()
{
  m_bOpenAfterRename = false;

  if (ListAssets->currentIndex().isValid())
  {
    ListAssets->edit(ListAssets->currentIndex());
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

  QMessageBox::StandardButton choice = ezQtUiServices::MessageBoxQuestion(ezFmt("Delete the selected file?\n\nThis operation cannot be undone."), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
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

void ezQtAssetBrowserWidget::OnImportAsAboutToShow()
{
  QMenu* pMenu = qobject_cast<QMenu*>(sender());

  if (!pMenu->actions().isEmpty())
    return;

  ezHybridArray<ezString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  ezSet<ezString> extensions;
  ezStringBuilder sExt;
  for (const auto& file : filesToImport)
  {
    sExt = file.GetFileExtension();
    sExt.ToLower();
    extensions.Insert(sExt);
  }

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  ezAssetDocumentGenerator::CreateGenerators(generators);
  ezHybridArray<ezAssetDocumentGenerator::ImportMode, 16> importModes;

  for (ezAssetDocumentGenerator* pGen : generators)
  {
    for (ezStringView ext : extensions)
    {
      if (pGen->SupportsFileType(ext))
      {
        pGen->GetImportModes({}, importModes);
        break;
      }
    }
  }

  for (const auto& mode : importModes)
  {
    QAction* act = pMenu->addAction(QIcon(ezMakeQString(mode.m_sIcon)), ezMakeQString(ezTranslate(mode.m_sName)));
    act->setData(ezMakeQString(mode.m_sName));
    connect(act, &QAction::triggered, this, &ezQtAssetBrowserWidget::OnImportAsClicked);
  }

  ezAssetDocumentGenerator::DestroyGenerators(generators);
}

void ezQtAssetBrowserWidget::OnImportAsClicked()
{
  ezHybridArray<ezString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  QAction* act = qobject_cast<QAction*>(sender());
  ezString sMode = qtToEzString(act->data().toString());

  ezHybridArray<ezAssetDocumentGenerator*, 16> generators;
  ezAssetDocumentGenerator::CreateGenerators(generators);

  ezHybridArray<ezAssetDocumentGenerator::ImportMode, 16> importModes;
  for (ezAssetDocumentGenerator* pGen : generators)
  {
    importModes.Clear();
    pGen->GetImportModes({}, importModes);

    for (const auto& mode : importModes)
    {
      if (mode.m_sName == sMode)
      {
        for (const ezString& file : filesToImport)
        {
          if (pGen->SupportsFileType(file))
          {
            pGen->Import(file, sMode, false).LogFailure();
          }
        }

        goto done;
      }
    }
  }

done:
  ezAssetDocumentGenerator::DestroyGenerators(generators);
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
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  {
    QAction* pAction = m.addAction(QIcon(":/GuiFoundation/Icons/SaveAll.svg"), QLatin1String("Re-save Assets in Folder"), this, SLOT(OnResaveAssets()));
    pAction->setToolTip("Opens every document and saves it. Used to get all documents to the latest version.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void ezQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  ezQtScopedBlockSignals block(TypeFilter);

  ezStringBuilder sFilter;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pFilter->SetShowNonImportableFiles(index == 0);
      m_pFilter->SetShowFiles(index == 0 || index == 1);
      ResetTypeFilter->setEnabled(index != 2);
      break;
    case Mode::AssetPicker:
      m_pFilter->SetShowNonImportableFiles(false);
      m_pFilter->SetShowFiles(false);
      ResetTypeFilter->setEnabled(index != 0);
      break;
    case Mode::FilePicker:
      m_pFilter->SetShowNonImportableFiles(true);
      m_pFilter->SetShowFiles(true);
      ResetTypeFilter->setEnabled(false);
      break;
  }


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

  ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
  pPreferences->m_bAssetBrowserShowItemsInSubFolders = m_pFilter->GetShowItemsInSubFolders();
}

void ezQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void ezQtAssetBrowserWidget::OnResaveAssets()
{
  if (QTreeWidgetItem* pCurrentItem = TreeFolderFilter->currentItem())
  {
    QModelIndex id = TreeFolderFilter->indexFromItem(pCurrentItem);
    ezStringBuilder sAbsPath = qtToEzString(id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString());

    ezAssetCurator::GetSingleton()->ResaveAllAssets(sAbsPath);
  }
}

void ezQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    bool bShowDocumentActions = false;

    if (m_Mode == Mode::Browser)
    {
      QString sTitle = "Open Selection";
      QIcon icon = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));

      bool bShowOpenWith = false;

      if (ListAssets->selectionModel()->selectedIndexes().count() == 1)
      {
        const QModelIndex firstItem = ListAssets->selectionModel()->selectedIndexes()[0];
        const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)firstItem.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(ezAssetBrowserItemFlags::Asset | ezAssetBrowserItemFlags::SubAsset))
        {
          sTitle = "Open Document";
          bShowDocumentActions = true;
        }
        else if (itemType.IsSet(ezAssetBrowserItemFlags::File))
        {
          sTitle = "Open File";
          bShowOpenWith = true;
        }
        else if (itemType.IsAnySet(ezAssetBrowserItemFlags::DataDirectory | ezAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));

      if (bShowOpenWith)
      {
        m.addAction(icon, "Open With...", this, SLOT(OnListOpenFileWith()));
      }
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));

    if (bShowDocumentActions)
    {
      m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/TransformAsset.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));

      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]()
        { OnListFindAllReferences(false); });
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]()
        { OnListFindAllReferences(true); });
    }
  }


  if (m_Mode == Mode::Browser && ListAssets->selectionModel()->hasSelection())
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

    // Rename
    {
      bool bCanRename = true;

      QModelIndex id = ListAssets->currentIndex();

      const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)id.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(ezAssetBrowserItemFlags::SubAsset | ezAssetBrowserItemFlags::DataDirectory))
      {
        bCanRename = false;
      }

      QAction* pRename = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Rename.svg")), QLatin1String("Rename"), this, SLOT(RenameCurrent()));
      pRename->setShortcut(QKeySequence("F2"));
      if (!bCanRename)
      {
        pRename->setEnabled(false);
        pRename->setToolTip("Sub-assets and data directories can't be renamed.");
      }
    }

    // Delete
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(DeleteSelection()));
      pDelete->setShortcut(QKeySequence("Del"));
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
      QMenu* imp = m.addMenu(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), "Import As");
      connect(imp, &QMenu::aboutToShow, this, &ezQtAssetBrowserWidget::OnImportAsAboutToShow);
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

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

void ezQtAssetBrowserWidget::OnListOpenFileWith()
{
  if (!ListAssets->currentIndex().isValid())
    return;

  ezString sPath = m_pModel->data(ListAssets->currentIndex(), ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  ezQtUiServices::OpenWith(sPath);
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
  sFilter.SetFormat("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
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

    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
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

    const ezBitflags<ezAssetBrowserItemFlags> itemType = (ezAssetBrowserItemFlags::Enum)index.data(ezQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    ezUuid guid = m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::SubAssetGuid).value<ezUuid>();
    Q_EMIT ItemSelected(guid, m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
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
  ezString sStartFileName = ezTranslate(sAssetType);
  ezString sExtension = pSender->property("Extension").toString().toUtf8().data();
  bool useSelection = pSender->property("UseSelection").toBool();

  QString sStartDir;

  // find path
  {
    if (TreeFolderFilter->selectionModel()->hasSelection())
    {
      auto idx = TreeFolderFilter->selectionModel()->selection().indexes()[0];
      sStartDir = TreeFolderFilter->itemFromIndex(idx)->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
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

        sStartFileName = temp.GetFileName();
      }
    }
  }

  if (sStartDir.isEmpty())
  {
    // this happens when the root node is selected
    sStartDir = ezToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  ezStringBuilder sNewAsset = qtToEzString(sStartDir);
  ezStringBuilder sBaseFileName;
  ezPathUtils::MakeValidFilename(sStartFileName, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (ezUInt32 i = 2; ezOSFile::ExistsFile(sNewAsset); i++)
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
  sNewPath.ChangeFileName(qtToEzString(sNewName));

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

    // it is necessary to flush the events queued on the main thread, otherwise opening the asset may not work as intended
    ezAssetCurator::GetSingleton()->MainThreadTick(true);

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

    ezInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      on_ListAssets_doubleClicked(idx);
    }
  }
}

void ezQtAssetBrowserWidget::ImportSelection()
{
  ezHybridArray<ezString, 4> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  ezAssetDocumentGenerator::ImportAssets(filesToImport);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
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

void ezQtAssetBrowserWidget::SetSelectedFile(ezStringView sAbsPath)
{
  if (sAbsPath.IsEmpty())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToFile", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, ezMakeQString(sAbsPath)));
}

void ezQtAssetBrowserWidget::OnScrollToItem(ezUuid preselectedAsset)
{
  const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(preselectedAsset);
  if (pSubAsset.isValid())
  {
    ezStringBuilder sPath = ezMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath()).toUtf8().data();
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
    m_pFilter->SetTextFilter("");
  }

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

void ezQtAssetBrowserWidget::OnScrollToFile(QString sPreselectedFile)
{
  ezStringBuilder sPath = sPreselectedFile.toUtf8().data();
  if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
  {
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
  }

  for (ezInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    QModelIndex idx = m_pModel->index(i, 0);

    if (m_pModel->data(idx, ezQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>() == sPreselectedFile)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void ezQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(ezStringView sFilters)
{
  m_sAllTypesFilter.Clear();

  if (!sFilters.IsEmpty())
  {
    ezStringBuilder sFilter;
    const ezStringBuilder sAllFilters(";", sFilters, ";");

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

void ezQtAssetBrowserWidget::UseFileExtensionFilters(ezStringView sFileExtensions)
{
  m_pFilter->SetFileExtensionFilters(sFileExtensions);
}

void ezQtAssetBrowserWidget::SetRequiredTag(ezStringView sRequiredTag)
{
  m_pFilter->SetRequiredTag(sRequiredTag);
}
