#include <PCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <QFileDialog>

QString ezQtExtractGeometryDlg::s_sDestinationFile;
bool ezQtExtractGeometryDlg::s_bOnlySelection = false;
int ezQtExtractGeometryDlg::s_iExtractionMode = (int)ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh;

ezQtExtractGeometryDlg::ezQtExtractGeometryDlg(QWidget* parent)

    : QDialog(parent)
{
  setupUi(this);

  ExtractionMode->clear();
  ExtractionMode->addItem("Render Mesh");
  ExtractionMode->addItem("Collision Mesh");
  ExtractionMode->addItem("Navmesh Obstacles");

  UpdateUI();
}

void ezQtExtractGeometryDlg::UpdateUI()
{
  DestinationFile->setText(s_sDestinationFile);
  ExtractOnlySelection->setChecked(s_bOnlySelection);
  ExtractionMode->setCurrentIndex(s_iExtractionMode);
}

void ezQtExtractGeometryDlg::QueryUI()
{
  s_sDestinationFile = DestinationFile->text();
  s_bOnlySelection = ExtractOnlySelection->isChecked();
  s_iExtractionMode = ExtractionMode->currentIndex();
}

void ezQtExtractGeometryDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();
    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}

void ezQtExtractGeometryDlg::on_BrowseButton_clicked()
{
  QString allFilters = "OBJ (*.obj)";
  QString sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Destination file"), s_sDestinationFile,
                                               allFilters, nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  DestinationFile->setText(sFile);
}
