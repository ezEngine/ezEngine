#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowser.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>

ezAssetBrowser::ezAssetBrowser(QWidget* parent) : QWidget(parent)
{
  setupUi(this);

  m_pModel = new ezAssetCuratorModel(this);

  ListAssets->setModel(m_pModel);
}

ezAssetBrowser::~ezAssetBrowser()
{
  ListAssets->setModel(nullptr);
}

void ezAssetBrowser::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  ezContainerWindow::CreateOrOpenDocument(false, m_pModel->data(index, Qt::UserRole + 1).toString().toUtf8().data());

}

