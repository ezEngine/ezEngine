#include <PCH.h>
#include <EditorPluginAssets/SceneImport/SceneImportDlg.moc.h>
#include <QDialogButtonBox>
#include <QPushButton>

ezQtSceneImportDlg::ezQtSceneImportDlg(QWidget *parent) : QDialog(parent)
{
  setupUi(this);

  // TODO: validator for input boxes.

  QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ezQtSceneImportDlg::on_accepted);
  QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, [this]() { reject(); });
  QObject::connect(browseButtonSourceFilename, &QPushButton::clicked, this, &ezQtSceneImportDlg::on_InputBrowse_clicked);
  QObject::connect(browseButtonOutputFilename, &QPushButton::click, this, &ezQtSceneImportDlg::on_OutputBrowse_clicked);
}

void ezQtSceneImportDlg::on_accepted()
{
  // Some validation.

  // Load model.

  // Create scene.

  // Import meshes.

  // Import nodes.

  accept();
}

void ezQtSceneImportDlg::on_InputBrowse_clicked()
{

}

void ezQtSceneImportDlg::on_OutputBrowse_clicked()
{

  //sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter), nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();
}
