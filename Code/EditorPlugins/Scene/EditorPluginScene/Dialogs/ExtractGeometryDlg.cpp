#include <EditorPluginScenePCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFileDialog>

QString ezQtExtractGeometryDlg::s_sDestinationFile;
bool ezQtExtractGeometryDlg::s_bOnlySelection = false;
int ezQtExtractGeometryDlg::s_iExtractionMode = (int)ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
int ezQtExtractGeometryDlg::s_iCoordinateSystem = 1;

ezQtExtractGeometryDlg::ezQtExtractGeometryDlg(QWidget* parent)

    : QDialog(parent)
{
  setupUi(this);

  ExtractionMode->clear();
  ExtractionMode->addItem("Render Mesh");
  ExtractionMode->addItem("Collision Mesh");
  ExtractionMode->addItem("Navmesh Obstacles");

  CoordinateSystem->clear();
  CoordinateSystem->addItem("Forward: +X, Right: +Y, Up: +Z (ez)");
  CoordinateSystem->addItem("Forward: -Z, Right: +X, Up: +Y (OpenGL/Maya)");
  CoordinateSystem->addItem("Forward: +Z, Right: +X, Up: +Y (D3D)");

  UpdateUI();
}

void ezQtExtractGeometryDlg::UpdateUI()
{
  DestinationFile->setText(s_sDestinationFile);
  ExtractOnlySelection->setChecked(s_bOnlySelection);
  ExtractionMode->setCurrentIndex(s_iExtractionMode);
  CoordinateSystem->setCurrentIndex(s_iCoordinateSystem);
}

void ezQtExtractGeometryDlg::QueryUI()
{
  s_sDestinationFile = DestinationFile->text();
  s_bOnlySelection = ExtractOnlySelection->isChecked();
  s_iExtractionMode = ExtractionMode->currentIndex();
  s_iCoordinateSystem = CoordinateSystem->currentIndex();
}

void ezQtExtractGeometryDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();

    if (!ezPathUtils::IsAbsolutePath(s_sDestinationFile.toUtf8().data()))
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning("Only absolute paths are allowed for the destination file.");
      return;
    }

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

ezMat3 ezQtExtractGeometryDlg::GetCoordinateSystemTransform()
{
  ezMat3 m;
  m.SetIdentity();

  switch (s_iCoordinateSystem)
  {
    case 0:
      break;

    case 1:
      m.SetRow(2, ezVec3(-1, 0, 0)); // forward
      m.SetRow(1, ezVec3(0, 0, 1));  // up
      m.SetRow(0, ezVec3(0, 1, 0));  // right
      break;

    case 2:
      m.SetRow(2, ezVec3(1, 0, 0)); // forward
      m.SetRow(1, ezVec3(0, 0, 1)); // up
      m.SetRow(0, ezVec3(0, 1, 0)); // right
      break;
  }

  return m;
}
