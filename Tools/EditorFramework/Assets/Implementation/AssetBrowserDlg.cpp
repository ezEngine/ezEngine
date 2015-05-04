#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <QSettings>

ezAssetBrowserDlg::ezAssetBrowserDlg(QWidget* parent, const char* szPreselectedAsset, const char* szVisibleFilters) : QDialog(parent)
{
  setupUi(this);

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
