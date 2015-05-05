#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <QSettings>
#include <QFileDialog>

ezAssetBrowserDlg::ezAssetBrowserDlg(QWidget* parent, const char* szPreselectedAsset, const char* szVisibleFilters) : QDialog(parent)
{
  setupUi(this);

  m_sSelectedPath = szPreselectedAsset;

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

void ezAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(QString sItemPath)
{
  m_sSelectedPath = sItemPath.toUtf8().data();
}

void ezAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(QString sItemPath)
{
  m_sSelectedPath = sItemPath.toUtf8().data();
  accept();
}

void ezAssetBrowserDlg::on_ButtonFileDialog_clicked()
{
  hide();

  static QString sLastPath;

  m_sSelectedPath.Clear();

  const QString sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open File"), sLastPath);

  if (sFile.isEmpty())
  {
    reject();
    return;
  }

  sLastPath = sFile;
  on_AssetBrowserWidget_ItemChosen(sFile);
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
