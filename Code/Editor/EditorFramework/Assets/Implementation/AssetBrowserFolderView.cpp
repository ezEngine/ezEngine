#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>


ezFileNameValidator::ezFileNameValidator(QObject* pParent, ezStringView sParentFolder, ezStringView sCurrentName)
  : QValidator(pParent)
  , m_sParentFolder(sParentFolder)
  , m_sCurrentName(sCurrentName)
{
}

QValidator::State ezFileNameValidator::validate(QString& ref_sInput, int& ref_iPos) const
{
  ezStringBuilder sTemp = ref_sInput.toUtf8().constData();
  if (sTemp.IsEmpty())
    return QValidator::State::Intermediate;
  if (ezPathUtils::ContainsInvalidFilenameChars(sTemp))
    return QValidator::State::Invalid;
  if (sTemp.StartsWith_NoCase(" ") || sTemp.EndsWith(" "))
    return QValidator::State::Intermediate;

  if (!m_sCurrentName.IsEmpty() && sTemp == m_sCurrentName)
    return QValidator::State::Acceptable;
  if (!m_sCurrentName.IsEmpty() && sTemp == m_sCurrentName.GetFileName())
    return QValidator::State::Acceptable;

  ezStringBuilder sAbsPath = m_sParentFolder;
  sAbsPath.AppendPath(sTemp);
  sAbsPath.Append(".", m_sCurrentName.GetFileExtension());

  if (ezOSFile::ExistsDirectory(sAbsPath) || ezOSFile::ExistsFile(sAbsPath))
    return QValidator::State::Intermediate;

  return QValidator::State::Acceptable;
}


ezFolderNameDelegate::ezFolderNameDelegate(QObject* pParent /*= nullptr*/)
  : QItemDelegate(pParent)
{
}

QWidget* ezFolderNameDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  ezStringBuilder sAbsPath = index.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new ezFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void ezFolderNameDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString sOldName = index.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  emit editingFinished(sOldName, pLineEdit->text());
}



eqQtAssetBrowserFolderView::eqQtAssetBrowserFolderView(QWidget* pParent)
  : QTreeWidget(pParent)
{
  viewport()->setAcceptDrops(true);
  setAcceptDrops(true);

  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);

  SetDialogMode(false);

  setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  auto pDelegate = new ezFolderNameDelegate(this);
  EZ_VERIFY(connect(pDelegate, &ezFolderNameDelegate::editingFinished, this, &eqQtAssetBrowserFolderView::OnFolderEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");
  setItemDelegate(pDelegate);

  ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(ezMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));

  EZ_VERIFY(connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemSelectionChanged())) != nullptr, "signal/slot connection failed");

  UpdateDirectoryTree();
}

eqQtAssetBrowserFolderView::~eqQtAssetBrowserFolderView()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));
  ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(ezMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
}


void eqQtAssetBrowserFolderView::SetFilter(ezQtAssetBrowserFilter* pFilter)
{
  m_pFilter = pFilter;
  EZ_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
}


void eqQtAssetBrowserFolderView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
  }
  else
  {
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
  }
}

void eqQtAssetBrowserFolderView::NewFolder()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  if (!currentItem())
    return;

  ezStringBuilder sPath = currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  ezStringBuilder sNewFolder = sPath;
  sNewFolder.AppendFormat("/NewFolder");

  for (ezUInt32 i = 2; ezOSFile::ExistsDirectory(sNewFolder); i++)
  {
    sNewFolder = sPath;
    sNewFolder.AppendFormat("/NewFolder{}", i);
  }

  if (ezFileSystem::CreateDirectoryStructure(sNewFolder).Succeeded())
  {
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sNewFolder);
    OnFlushFileSystemEvents();

    if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewFolder))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewFolder, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
        OnItemSelectionChanged(); // make sure the path filter is set to the new folder
        editItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName)
{
  ezStringBuilder sPath = sAbsPath.toUtf8().data();
  ezStringBuilder sNewPath = sPath;
  sNewPath.ChangeFileNameAndExtension(sNewName.toUtf8().data());

  if (sPath != sNewPath)
  {
    if (ezOSFile::MoveFileOrDirectory(sPath, sNewPath).Failed())
    {
      ezLog::Error("Failed to rename '{}' to '{}'", sPath, sNewPath);
      return;
    }

    ezFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
    OnFlushFileSystemEvents();

    if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewPath))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewPath, topLevelItem(0), {});
      if (pItem)
      {
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        topLevelItem(0)->setSelected(false);
        setCurrentItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler(const ezFolderChangedEvent& e)
{
  EZ_LOCK(m_FolderStructureMutex);
  m_QueuedFolderEvents.PushBack(e);

  QTimer::singleShot(0, this, SLOT(OnFlushFileSystemEvents()));
}

void eqQtAssetBrowserFolderView::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectClosed:
    {
      // remove project structure from asset browser
      ClearDirectoryTree();
    }
    break;
    default:
      break;
  }
}

void eqQtAssetBrowserFolderView::dragMoveEvent(QDragMoveEvent* e)
{
  QTreeWidget::dragMoveEvent(e);

  ezHybridArray<ezString, 1> files;
  ezString sTarget;
  ezStatus res = canDrop(e, files, sTarget);
  if (res.Failed())
  {
    ezQtUiServices::ShowGlobalStatusBarMessage(res.GetMessageString().GetView());
    e->ignore();
  }
  else
  {
    ezQtUiServices::ShowGlobalStatusBarMessage({});
  }
}

void eqQtAssetBrowserFolderView::mouseMoveEvent(QMouseEvent* e)
{
  // only allow dragging with left mouse button
  if (state() == DraggingState && !e->buttons().testFlag(Qt::MouseButton::LeftButton))
  {
    return;
  }

  QTreeWidget::mouseMoveEvent(e);
}

ezStatus eqQtAssetBrowserFolderView::canDrop(QDropEvent* e, ezDynamicArray<ezString>& out_files, ezString& out_sTargetFolder)
{
  if (!e->mimeData()->hasFormat("application/ezEditor.files"))
  {
    return ezStatus(EZ_FAILURE);
  }

  DropIndicatorPosition dropIndicator = dropIndicatorPosition();
  if (dropIndicator != QAbstractItemView::OnItem)
  {
    return ezStatus(EZ_FAILURE);
  }

  auto action = e->dropAction();
  if (action != Qt::MoveAction)
  {
    return ezStatus(EZ_FAILURE);
  }

  out_files.Clear();
  QByteArray encodedData = e->mimeData()->data("application/ezEditor.files");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  ezHybridArray<QString, 1> files;
  stream >> files;

  QModelIndex dropIndex = indexAt(e->position().toPoint());
  if (dropIndex.isValid())
  {
    QString sAbsTarget = dropIndex.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    out_sTargetFolder = qtToEzString(sAbsTarget);

    for (const QString& sFile : files)
    {
      ezString sFileToMove = qtToEzString(sFile);
      out_files.PushBack(sFileToMove);

      if (ezPathUtils::IsSubPath(sFileToMove, out_sTargetFolder))
      {
        return ezStatus(ezFmt("Can't move '{}' into its own sub-folder '{}'", sFileToMove, out_sTargetFolder));
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

void eqQtAssetBrowserFolderView::dropEvent(QDropEvent* e)
{
  ezQtUiServices::ShowGlobalStatusBarMessage({});
  ezHybridArray<ezString, 1> files;
  ezString sTargetFolder;
  // Always accept and call base class to end the drop operation as a no-op in the base class.
  e->accept();
  QTreeWidget::dropEvent(e);
  if (canDrop(e, files, sTargetFolder).Failed())
  {
    return;
  }

  QMessageBox::StandardButton choice = ezQtUiServices::MessageBoxQuestion(ezFmt("Move {} items into '{}'?", files.GetCount(), sTargetFolder), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No);
  if (choice == QMessageBox::StandardButton::No)
    return;

  ezStringBuilder sNewLocation;
  for (const ezString& sFile : files)
  {
    sNewLocation = sTargetFolder;
    sNewLocation.AppendPath(ezPathUtils::GetFileNameAndExtension(sFile));
    if (ezOSFile::MoveFileOrDirectory(sFile, sNewLocation).Failed())
    {
      ezLog::Error("Failed to move '{}' to '{}'", sFile, sNewLocation);
    }
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sNewLocation);
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sFile);
  }
  OnFlushFileSystemEvents();

  if (e->source() == this && files.GetCount() == 1)
  {
    if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewLocation))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewLocation, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
      }
    }
  }
}


QStringList eqQtAssetBrowserFolderView::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.files";
  return types;
}


Qt::DropActions eqQtAssetBrowserFolderView::supportedDropActions() const
{
  return Qt::DropAction::MoveAction | Qt::DropAction::CopyAction;
}

QMimeData* eqQtAssetBrowserFolderView::mimeData(const QList<QTreeWidgetItem*>& items) const
{
  ezHybridArray<QString, 1> files;
  for (const QTreeWidgetItem* pItem : items)
  {
    QModelIndex id = indexFromItem(pItem);
    QString text = id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    files.PushBack(text);
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/ezEditor.files", encodedData);
  return mimeData;
}

void eqQtAssetBrowserFolderView::keyPressEvent(QKeyEvent* e)
{
  QTreeWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && !m_bDialogMode)
  {
    e->accept();
    DeleteFolder();
    return;
  }
}

void eqQtAssetBrowserFolderView::DeleteFolder()
{
  if (QTreeWidgetItem* pCurrentItem = currentItem())
  {
    QModelIndex id = indexFromItem(pCurrentItem);
    QString sQtAbsPath = id.data(ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    ezString sAbsPath = qtToEzString(sQtAbsPath);
    QMessageBox::StandardButton choice = ezQtUiServices::MessageBoxQuestion(ezFmt("Do you want to delete the folder\n'{}'?", sAbsPath), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
    if (choice == QMessageBox::StandardButton::Cancel)
      return;

    if (!QFile::moveToTrash(sQtAbsPath))
    {
      ezLog::Error("Failed to delete folder '{}'", sAbsPath);
    }
    ezFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void eqQtAssetBrowserFolderView::OnFlushFileSystemEvents()
{
  EZ_LOCK(m_FolderStructureMutex);

  for (const auto& e : m_QueuedFolderEvents)
  {
    switch (e.m_Type)
    {
      case ezFolderChangedEvent::Type::FolderAdded:
      {
        BuildDirectoryTree(e.m_Path, e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "", false);
      }
      break;

      case ezFolderChangedEvent::Type::FolderRemoved:
      {
        RemoveDirectoryTreeItem(e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "");
      }
      break;

      case ezFolderChangedEvent::Type::ModelReset:
        UpdateDirectoryTree();
        break;

      default:
        break;
    }
  }

  m_QueuedFolderEvents.Clear();
}

void eqQtAssetBrowserFolderView::mouseDoubleClickEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->ignore();
    return;
  }

  QTreeWidget::mouseDoubleClickEvent(e);
}

void eqQtAssetBrowserFolderView::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->ignore();
    return;
  }

  QModelIndex inx = indexAt(e->pos());
  if (!inx.isValid())
    return;

  QTreeWidget::mousePressEvent(e);
}

void eqQtAssetBrowserFolderView::OnItemSelectionChanged()
{
  if (m_bTreeSelectionChangeInProgress)
    return;

  ezStringBuilder sCurPath;

  if (!selectedItems().isEmpty())
  {
    sCurPath = selectedItems()[0]->data(0, ezQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void eqQtAssetBrowserFolderView::OnPathFilterChanged()
{
  const QString sPath = ezMakeQString(m_pFilter->GetPathFilter());

  if (topLevelItemCount() == 1)
  {
    if (m_bTreeSelectionChangeInProgress)
      return;

    m_bTreeSelectionChangeInProgress = true;
    clearSelection();
    SelectPathFilter(topLevelItem(0), sPath);
    m_bTreeSelectionChangeInProgress = false;
  }
}


void eqQtAssetBrowserFolderView::TreeOpenExplorer()
{
  if (!currentItem())
    return;

  ezStringBuilder sPath = currentItem()->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  ezQtUiServices::OpenInExplorer(sPath, false);
}

bool eqQtAssetBrowserFolderView::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, ezQtAssetBrowserModel::UserRoles::RelativePath).toString() == sPath)
  {
    pParent->setSelected(true);
    setCurrentIndex(indexFromItem(pParent));
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
void eqQtAssetBrowserFolderView::UpdateDirectoryTree()
{
  ezQtScopedBlockSignals block(this);

  if (topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);

    selectionModel()->select(indexFromItem(pNewParent), QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }

  auto Folders = ezFileSystemModel::GetSingleton()->GetFolders();

  if (m_uiKnownAssetFolderCount == Folders->GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders->GetCount();

  ezStringBuilder tmp;

  for (const auto& sDir : *Folders)
  {
    BuildDirectoryTree(sDir.Key(), sDir.Key().GetDataDirParentRelativePath(), topLevelItem(0), "", false);
  }

  setSortingEnabled(true);
  sortItems(0, Qt::SortOrder::AscendingOrder);
}


void eqQtAssetBrowserFolderView::ClearDirectoryTree()
{
  clear();
  m_uiKnownAssetFolderCount = 0;
}

void eqQtAssetBrowserFolderView::BuildDirectoryTree(const ezDataDirPath& path, ezStringView sCurPath, QTreeWidgetItem* pParent, ezStringView sCurPathToItem, bool bIsHidden)
{
  if (sCurPath.IsEmpty())
    return;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  ezString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = ezStringView(sCurPath.GetStartPointer(), szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  ezStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = ezMakeQString(sFolderName.GetView());

  if (sQtFolderName == "AssetCache")
    return;

  for (ezInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  { // #TODO_ASSET data for folder

    QString sPathAbs = pParent->data(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    QString sPathRel = pParent->data(0, ezQtAssetBrowserModel::UserRoles::RelativePath).toString();

    if (sPathAbs.isEmpty())
    {
      sPathAbs = ezMakeQString(path.GetAbsolutePath());
      sPathRel = ezMakeQString(path.GetDataDirParentRelativePath());
    }
    else
    {
      sPathAbs += "/" + sQtFolderName;
      sPathRel += "/" + sQtFolderName;
    }

    const bool bIsDataDir = sCurPathToItem.IsEmpty();
    pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, sQtFolderName);
    pNewParent->setData(0, ezQtAssetBrowserModel::UserRoles::AbsolutePath, sPathAbs);
    pNewParent->setData(0, ezQtAssetBrowserModel::UserRoles::RelativePath, sPathRel);
    ezBitflags<ezAssetBrowserItemFlags> flags = bIsDataDir ? ezAssetBrowserItemFlags::DataDirectory : ezAssetBrowserItemFlags::Folder;
    pNewParent->setData(0, ezQtAssetBrowserModel::UserRoles::ItemFlags, (int)flags.GetValue());
    pNewParent->setIcon(0, ezQtUiServices::GetCachedIconResource(bIsDataDir ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg"));
    if (!bIsDataDir)
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsDragEnabled | Qt::ItemFlag::ItemIsDropEnabled);
    else
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsDropEnabled);

    if (bIsHidden)
    {
      pNewParent->setForeground(0, QColor::fromRgba(qRgb(110, 110, 120)));
    }

    pParent->addChild(pNewParent);
  }

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(path, szNextSep + 1, pNewParent, sCurPath2, bIsHidden);
}

void eqQtAssetBrowserFolderView::RemoveDirectoryTreeItem(ezStringView sCurPath, QTreeWidgetItem* pParent, ezStringView sCurPathToItem)
{
  if (QTreeWidgetItem* pTreeItem = FindDirectoryTreeItem(sCurPath, pParent, sCurPathToItem))
  {
    delete pTreeItem;
  }
}


QTreeWidgetItem* eqQtAssetBrowserFolderView::FindDirectoryTreeItem(ezStringView sCurPath, QTreeWidgetItem* pParent, ezStringView sCurPathToItem)
{
  if (sCurPath.IsEmpty())
    return nullptr;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  ezString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = ezStringView(sCurPath.GetStartPointer(), szNextSep);

  ezStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = ezMakeQString(sFolderName.GetView());

  for (ezInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  return nullptr;

godown:

  if (szNextSep == nullptr)
  {
    return pNewParent;
  }

  return FindDirectoryTreeItem(szNextSep + 1, pNewParent, sCurPath2);
}
