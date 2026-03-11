#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

EZ_IMPLEMENT_SINGLETON(ezQtAssetBrowserPanel);

ezQtAssetBrowserPanel::ezQtAssetBrowserPanel(ads::CDockManager* pDockManager)
  : ezQtApplicationPanel(pDockManager, "Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setFeature(ads::CDockWidget::DockWidgetClosable, false);

  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(pDummy);

  setIcon(ezQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Asset.svg"));
  setWindowTitle(ezMakeQString(ezTranslate("Panel.AssetBrowser")));

  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemChosen, this, &ezQtAssetBrowserPanel::SlotAssetChosen) != nullptr,
    "signal/slot connection failed");
  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemSelected, this, &ezQtAssetBrowserPanel::SlotAssetSelected) != nullptr,
    "signal/slot connection failed");
  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemCleared, this, &ezQtAssetBrowserPanel::SlotAssetCleared) != nullptr,
    "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
}

ezQtAssetBrowserPanel::~ezQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void ezQtAssetBrowserPanel::SlotAssetChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags)
{
  if (guid.IsValid())
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
  }
  else
  {
    ezQtUiServices::OpenFileInDefaultProgram(qtToEzString(sAssetPathAbsolute)).IgnoreResult();
  }
}

void ezQtAssetBrowserPanel::SlotAssetSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, ezUInt8 uiAssetBrowserItemFlags)
{
  m_LastSelected = guid;
}

void ezQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected = ezUuid::MakeInvalid();
}
