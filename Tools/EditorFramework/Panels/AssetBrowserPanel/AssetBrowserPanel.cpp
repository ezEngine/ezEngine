#include <PCH.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QStatusBar>
#include <QSettings>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>

EZ_IMPLEMENT_SINGLETON(ezQtAssetBrowserPanel);

ezQtAssetBrowserPanel::ezQtAssetBrowserPanel()
  : ezQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  m_pStatusBar = new QStatusBar(this);
  m_pStatusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  m_pCuratorControl = new ezQtCuratorControl(nullptr);

  m_pStatusBar->addPermanentWidget(m_pCuratorControl);

  dockWidgetContents->layout()->addWidget(m_pStatusBar);

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


