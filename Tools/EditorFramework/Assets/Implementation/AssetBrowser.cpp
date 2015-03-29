#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowser.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <QVBoxLayout>

ezAssetBrowser::ezAssetBrowser(QWidget* parent) : QWidget(parent)
{
  setupUi(this);

  m_pModel = new ezAssetCuratorModel(this);

  ListAssets->setModel(m_pModel);

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    RootLayout->insertWidget(0, pToolBar);
  }
}

ezAssetBrowser::~ezAssetBrowser()
{
  ListAssets->setModel(nullptr);
}

void ezAssetBrowser::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  ezContainerWindow::CreateOrOpenDocument(false, m_pModel->data(index, Qt::UserRole + 1).toString().toUtf8().data());

}

