#include <EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <QStatusBar>

EZ_IMPLEMENT_SINGLETON(ezQtAssetBrowserPanel);

ezQtAssetBrowserPanel::ezQtAssetBrowserPanel()
  : ezQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  setupUi(this);

  m_pStatusBar = new QStatusBar(nullptr);
  m_pStatusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  m_pCuratorControl = new ezQtCuratorControl(nullptr);

  m_pStatusBar->addPermanentWidget(m_pCuratorControl);

  dockWidgetContents->layout()->addWidget(m_pStatusBar);
  setWidget(dockWidgetContents);

  setIcon(ezQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Asset16.png"));
  setWindowTitle(QString::fromUtf8(ezTranslate("Panel.AssetBrowser")));

  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemChosen, this, &ezQtAssetBrowserPanel::SlotAssetChosen) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemSelected, this, &ezQtAssetBrowserPanel::SlotAssetSelected) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(AssetBrowserWidget, &ezQtAssetBrowserWidget::ItemCleared, this, &ezQtAssetBrowserPanel::SlotAssetCleared) != nullptr, "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
}

ezQtAssetBrowserPanel::~ezQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void ezQtAssetBrowserPanel::SlotAssetChosen(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
}

void ezQtAssetBrowserPanel::SlotAssetSelected(ezUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_LastSelected = guid;
}

void ezQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected.SetInvalid();
}
