#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <QSettings>
#include <QFileDialog>

ezAssetBrowserDlg::ezAssetBrowserDlg(QWidget* parent, const char* szPreselectedAsset, const char* szVisibleFilters) : QDialog(parent)
{
  setupUi(this);

  /// \todo Implement this
  //m_sSelectedPath = szPreselectedAsset;

  AssetBrowserWidget->SetSelectedAsset(szPreselectedAsset);
  AssetBrowserWidget->ShowOnlyTheseTypeFilters(szVisibleFilters);

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
}

ezAssetBrowserDlg::~ezAssetBrowserDlg()
{
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

void ezAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_sSelectedAssetGuid = sAssetGUID.toUtf8().data();
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();
}

void ezAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(QString sAssetGUID, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_sSelectedAssetGuid = sAssetGUID.toUtf8().data();
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  accept();
}

#include <EditorFramework/EditorApp/EditorApp.moc.h>

void ezAssetBrowserDlg::on_ButtonFileDialog_clicked()
{
  hide();

  static QString sLastPath;

  m_sSelectedAssetGuid.Clear();
  m_sSelectedAssetPathRelative.Clear();
  m_sSelectedAssetPathAbsolute.Clear();

  const QString sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open File"), sLastPath);

  if (sFile.isEmpty())
  {
    reject();
    return;
  }

  m_sSelectedAssetPathAbsolute = sFile.toUtf8().data();
  m_sSelectedAssetPathRelative = m_sSelectedAssetPathAbsolute;

  if (!ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(m_sSelectedAssetPathRelative))
  {
    // \todo Message Box: Invalid Path

    //reject();
    //return;
  }

  sLastPath = sFile;
  on_AssetBrowserWidget_ItemChosen("", QString::fromUtf8(m_sSelectedAssetPathRelative.GetData()), sFile);
}

void ezAssetBrowserDlg::on_ButtonOk_clicked()
{
  /// \todo Deactivate Ok button, when nothing is selectable

  accept();
}

void ezAssetBrowserDlg::on_ButtonCancel_clicked()
{
  reject();
}
