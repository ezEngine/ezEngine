#include <PCH.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QSettings>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

EZ_IMPLEMENT_SINGLETON(ezQtAssetBrowserPanel);

ezQtAssetBrowserPanel::ezQtAssetBrowserPanel() 
  : ezQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  setWindowIcon(ezUIServices::GetCachedIconResource(":/EditorFramework/Icons/Asset16.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.AssetBrowser")));

  EZ_VERIFY(connect(AssetBrowserWidget, SIGNAL(ItemChosen(QString, QString, QString)), this, SLOT(SlotAssetChosen(QString, QString, QString))) != nullptr, "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
  
}

ezQtAssetBrowserPanel::~ezQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void ezQtAssetBrowserPanel::SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  ezQtEditorApp::GetSingleton()->OpenDocument(sAssetPathAbsolute.toUtf8().data());
}