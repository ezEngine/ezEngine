#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>

ezAssetBrowserDlg::ezAssetBrowserDlg(QWidget* parent, const char* szPreselectedAsset, const char* szVisibleFilters) : QDialog(parent)
{
  setupUi(this);

  AssetBrowserWidget->SetSelectedAsset(szPreselectedAsset);
  AssetBrowserWidget->ShowOnlyTheseTypeFilters(szVisibleFilters);
}

ezAssetBrowserDlg::~ezAssetBrowserDlg()
{
}
