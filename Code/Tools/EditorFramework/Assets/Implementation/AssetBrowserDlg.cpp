#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <QFileDialog>
#include <QSettings>

bool ezQtAssetBrowserDlg::s_bShowItemsInSubFolder = true;
bool ezQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool ezQtAssetBrowserDlg::s_bSortByRecentUse = true;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_sTextFilter;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_sPathFilter;
ezMap<ezString, ezString> ezQtAssetBrowserDlg::s_sTypeFilter;

ezQtAssetBrowserDlg::ezQtAssetBrowserDlg(QWidget* parent, const ezUuid& preselectedAsset, const char* szVisibleFilters)
    : QDialog(parent)
{
  setupUi(this);

  m_sVisibleFilters = szVisibleFilters;
  ButtonSelect->setEnabled(false);

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  if (!ezStringUtils::IsEqual(szVisibleFilters, ";;")) // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(szVisibleFilters);
  }

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());
  }
  Settings.endGroup();

  AssetBrowserWidget->SetDialogMode();
  AssetBrowserWidget->RestoreState("AssetBrowserDlg");
  AssetBrowserWidget->GetAssetBrowserFilter()->SetSortByRecentUse(s_bSortByRecentUse);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInSubFolders(s_bShowItemsInSubFolder);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInHiddenFolders(s_bShowItemsInHiddenFolder);

  if (!s_sTextFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTextFilter(s_sTextFilter[m_sVisibleFilters]);

  if (!s_sPathFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetPathFilter(s_sPathFilter[m_sVisibleFilters]);

  if (!s_sTypeFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTypeFilter(s_sTypeFilter[m_sVisibleFilters]);

  AssetBrowserWidget->SearchWidget->setFocus();
}

ezQtAssetBrowserDlg::~ezQtAssetBrowserDlg()
{
  s_bShowItemsInSubFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInSubFolders();
  s_bShowItemsInHiddenFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInHiddenFolders();
  s_bSortByRecentUse = AssetBrowserWidget->GetAssetBrowserFilter()->GetSortByRecentUse();
  s_sTextFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTextFilter();
  s_sPathFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetPathFilter();
  s_sTypeFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTypeFilter();

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

void ezQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  ButtonSelect->setEnabled(m_SelectedAssetGuid.IsValid());
}

void ezQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
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

void ezQtAssetBrowserDlg::on_ButtonFileDialog_clicked()
{
  hide();

  static QString sLastPath;

  m_SelectedAssetGuid = ezUuid();
  m_sSelectedAssetPathRelative.Clear();
  m_sSelectedAssetPathAbsolute.Clear();

  const QString sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open File"), sLastPath, QString(),
                                                     nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
  {
    reject();
    return;
  }

  m_sSelectedAssetPathAbsolute = sFile.toUtf8().data();
  m_sSelectedAssetPathRelative = m_sSelectedAssetPathAbsolute;

  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(m_sSelectedAssetPathRelative))
  {
    // \todo Message Box: Invalid Path

    // reject();
    // return;
  }

  sLastPath = sFile;
  on_AssetBrowserWidget_ItemChosen(ezUuid(), QString::fromUtf8(m_sSelectedAssetPathRelative.GetData()), sFile);
}

void ezQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  /// \todo Deactivate Ok button, when nothing is selectable

  accept();
}
