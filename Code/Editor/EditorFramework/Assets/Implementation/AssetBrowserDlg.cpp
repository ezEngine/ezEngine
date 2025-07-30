#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

bool ezQtAssetBrowserDlg::s_bShowItemsInSubFolder = true;
bool ezQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool ezQtAssetBrowserDlg::s_bSortByRecentUse = true;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_TextFilter;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_PathFilter;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_TypeFilter;

void ClampWindowGeometryToScreens(QRect& ref_windowGeometry)
{
  const QList<QScreen*> screens = QGuiApplication::screens();

  for (QScreen* screen : screens)
  {
    const QRect screenGeom = screen->availableGeometry();
    if (screenGeom.intersects(ref_windowGeometry))
      return;
  }

  const QRect primaryGeom = QGuiApplication::primaryScreen()->availableGeometry();

  const QSize size = ref_windowGeometry.size();
  ref_windowGeometry.setLeft(ezMath::Clamp(ref_windowGeometry.left(), primaryGeom.left(), primaryGeom.right() - ref_windowGeometry.width()));
  ref_windowGeometry.setTop(ezMath::Clamp(ref_windowGeometry.top(), primaryGeom.top(), primaryGeom.bottom() - ref_windowGeometry.height()));
  ref_windowGeometry.setSize(size);
}

void ezQtAssetBrowserDlg::Init(QWidget* pParent)
{
  setupUi(this);

  ButtonSelect->setEnabled(false);

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

    QRect windowGeometry;
    windowGeometry.setTopLeft(Settings.value("WindowPosition", pos()).toPoint());
    windowGeometry.setSize(Settings.value("WindowSize", size()).toSize());
    ClampWindowGeometryToScreens(windowGeometry);

    move(windowGeometry.topLeft());
    resize(windowGeometry.size());
  }
  Settings.endGroup();

  AssetBrowserWidget->RestoreState("AssetBrowserDlg");
  AssetBrowserWidget->GetAssetBrowserFilter()->SetSortByRecentUse(s_bSortByRecentUse);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInSubFolders(s_bShowItemsInSubFolder);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInHiddenFolders(s_bShowItemsInHiddenFolder);

  if (!s_TextFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTextFilter(s_TextFilter[m_sVisibleFilters]);

  if (!s_PathFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetPathFilter(s_PathFilter[m_sVisibleFilters]);

  if (!s_TypeFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTypeFilter(s_TypeFilter[m_sVisibleFilters]);
}

ezQtAssetBrowserDlg::ezQtAssetBrowserDlg(QWidget* pParent, const ezUuid& preselectedAsset, ezStringView sVisibleFilters, ezStringView sWindowTitle, ezStringView sRequiredTag)
  : QDialog(pParent)
{
  {
    ezStringBuilder temp = sVisibleFilters;
    ezHybridArray<ezStringView, 4> compTypes;
    temp.Split(false, compTypes, ";");
    ezStringBuilder allFiltered = sVisibleFilters;

    for (const auto& descIt : ezAssetDocumentManager::GetAllDocumentDescriptors())
    {
      const ezDocumentTypeDescriptor* pType = descIt.Value();
      for (ezStringView ct : compTypes)
      {
        if (pType->m_CompatibleTypes.Contains(ct))
        {
          allFiltered.Append(";", pType->m_sDocumentTypeName, ";");
        }
      }
    }

    m_sVisibleFilters = allFiltered;
    m_sRequiredTag = sRequiredTag;
  }
  Init(pParent);

  AssetBrowserWidget->SetMode(ezQtAssetBrowserWidget::Mode::AssetPicker);

  if (m_sVisibleFilters != ";;") // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(m_sVisibleFilters);
  }

  AssetBrowserWidget->SetRequiredTag(m_sRequiredTag);

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  AssetBrowserWidget->SearchWidget->setFocus();

  if (!sWindowTitle.IsEmpty())
  {
    setWindowTitle(ezMakeQString(sWindowTitle));
  }
}

ezQtAssetBrowserDlg::ezQtAssetBrowserDlg(QWidget* pParent, ezStringView sWindowTitle, ezStringView sPreselectedFileAbs, ezStringView sFileExtensions)
  : QDialog(pParent)
{
  m_sVisibleFilters = sFileExtensions;

  Init(pParent);

  ezStringBuilder title(sFileExtensions, ")");
  title.ReplaceAll(";", "; ");
  title.ReplaceAll("  ", " ");
  title.PrependFormat("{} (", sWindowTitle);
  setWindowTitle(ezMakeQString(title));

  AssetBrowserWidget->SetMode(ezQtAssetBrowserWidget::Mode::FilePicker);
  AssetBrowserWidget->UseFileExtensionFilters(sFileExtensions);

  ezStringBuilder sParentRelPath = sPreselectedFileAbs;
  if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sParentRelPath))
  {
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTemporaryPinnedItem(sParentRelPath);
  }

  AssetBrowserWidget->SetSelectedFile(sPreselectedFileAbs);

  AssetBrowserWidget->SearchWidget->setFocus();
}

ezQtAssetBrowserDlg::~ezQtAssetBrowserDlg()
{
  s_bShowItemsInSubFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInSubFolders();
  s_bShowItemsInHiddenFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInHiddenFolders();
  s_bSortByRecentUse = AssetBrowserWidget->GetAssetBrowserFilter()->GetSortByRecentUse();
  s_TextFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTextFilter();
  s_PathFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetPathFilter();
  s_TypeFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTypeFilter();

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }
  Settings.endGroup();

  AssetBrowserWidget->SaveState("AssetBrowserDlg");
}

void ezQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  const ezBitflags<ezAssetBrowserItemFlags> flags = (ezAssetBrowserItemFlags::Enum)uiAssetBrowserItemFlags;

  ButtonSelect->setEnabled(flags.IsAnySet(ezAssetBrowserItemFlags::Asset | ezAssetBrowserItemFlags::SubAsset | ezAssetBrowserItemFlags::File));
}

void ezQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  accept();
}

void ezQtAssetBrowserDlg::on_AssetBrowserWidget_ItemCleared()
{
  ButtonSelect->setEnabled(false);
}

void ezQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  accept();
}
