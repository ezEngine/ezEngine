#include <PCH.h>
#include <EditorPluginAssets/SceneImport/SceneImportDlg.moc.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>
#include <EditorPluginAssets/ModelImporter/Mesh.h>
#include <EditorPluginAssets/ModelImporter/Node.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>

#include <Foundation/Types/ScopeExit.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

ezQtSceneImportDlg::ezQtSceneImportDlg(QWidget *parent) : QDialog(parent)
{
  setupUi(this);

  QObject::connect(dialogButtonBox, &QDialogButtonBox::accepted, this, &ezQtSceneImportDlg::on_accepted);
  QObject::connect(dialogButtonBox, &QDialogButtonBox::rejected, this, [this]() { reject(); });
  QObject::connect(browseButtonSourceFilename, &QPushButton::clicked, this, &ezQtSceneImportDlg::on_InputBrowse_clicked);
  QObject::connect(browseButtonOutputFilename, &QPushButton::clicked, this, &ezQtSceneImportDlg::on_OutputBrowse_clicked);
  QObject::connect(lineEditSourceFilename, &QLineEdit::textChanged, this, &ezQtSceneImportDlg::validatePaths);
  QObject::connect(lineEditOutputFilename, &QLineEdit::textChanged, this, &ezQtSceneImportDlg::validatePaths);

  validatePaths();
}

void ezQtSceneImportDlg::on_accepted()
{
  ezString inputFilename = lineEditSourceFilename->text().toUtf8().data();
  ezString outputFilename = lineEditOutputFilename->text().toUtf8().data();


  // Clear cache when we are done here.
  EZ_SCOPE_EXIT(ezModelImporter::Importer::GetSingleton()->ClearCachedScenes());

  // Load model.
  ezSharedPtr<ezModelImporter::Scene> rawScene = ezModelImporter::Importer::GetSingleton()->ImportScene(inputFilename, true);
  if (!rawScene)
  {
    QMessageBox::critical(this, "Failed to load import", "Failed to load the input model file. Details can be found in the editor log.");
    return;
  }

  // Create scene.
  ezAssetDocument* sceneDocument = ezDynamicCast<ezAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, outputFilename, false, false));
  if (!sceneDocument)
  {
    QMessageBox::critical(this, "Failed to load import", "Failed to create scene document.");
    return;
  }

  // Create meshes.
  ezDynamicArray<ezUuid> meshGuids;
  meshGuids.Reserve(rawScene->GetMeshes().GetCount());

  ezStringBuilder meshDirectory = outputFilename;
  meshDirectory = meshDirectory.GetFileDirectory();
  meshDirectory.AppendPath("Meshes");
  for (auto meshIt=rawScene->GetMeshes().GetIterator(); meshIt.IsValid(); ++meshIt)
  {
    // Figure out a good filename.
    ezStringBuilder validFileMeshName;
    if (ezPathUtils::MakeValidFilename(meshIt.Value()->m_Name, '_', validFileMeshName).Failed())
    {
      ezLog::Error("Failed to create a filename for mesh '%s'", meshIt.Value()->m_Name.GetData());
      continue;
    }
    ezStringBuilder meshFilename = meshDirectory;
    meshFilename.AppendPath(validFileMeshName);
    meshFilename.Append(".ezMeshAsset");

    // Create document.
    ezMeshAssetDocument* meshDocument = ezDynamicCast<ezMeshAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateOrOpenDocument(true, meshFilename, false, false));
    if (!meshDocument)
    {
      ezLog::Error("Failed to create a document for mesh '%s' in '%s'", meshIt.Value()->m_Name.GetData(), meshFilename.GetData());
      continue;
    }

    // Set mesh.
    {
      ezObjectAccessorBase* pAccessor = meshDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Set Mesh and Submesh (Scene Import)");

      ezDocumentObject* pMeshAsset = meshDocument->GetPropertyObject();
      pAccessor->SetValue(pMeshAsset, "Mesh File", inputFilename.GetData()).LogFailure();
      pAccessor->SetValue(pMeshAsset, "Submesh Name", meshIt.Value()->m_Name).LogFailure();
      pAccessor->SetValue(pMeshAsset, "Use Subfolder for Material Import", false).LogFailure();

      pAccessor->FinishTransaction();
    }

    // Transform.
    {
      meshDocument->SaveDocument();
      ezAssetCurator::GetSingleton()->TransformAsset(meshDocument->GetGuid());
      meshDocument->SaveDocument(); // Save again since transform may change properties!
      meshGuids.PushBack(meshDocument->GetGuid());
      meshDocument->GetDocumentManager()->CloseDocument(meshDocument);
    }
  }

  // Create nodes.
  {
    // todo.
  }

  accept();
}

void ezQtSceneImportDlg::validatePaths()
{
  // Todo more validation.

  ezString inputFilename = lineEditSourceFilename->text().toUtf8().data();
  ezString outputFilename = lineEditOutputFilename->text().toUtf8().data();
  if (inputFilename.IsEmpty() || outputFilename.IsEmpty() ||
      ezPathUtils::GetFileExtension(outputFilename).IsEqual_NoCase("ezScene") == false)
  {
    dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  }
  else
  {
    dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  }
}

void ezQtSceneImportDlg::on_InputBrowse_clicked()
{
  QString filter = "Supported Model Format (";
  ezHashSet<ezString> supportedTypeList = ezModelImporter::Importer::GetSingleton()->GetSupportedTypes();
  for (const ezString& type : supportedTypeList)
  {
    filter.push_back(" *.");
    filter.push_back(type.GetData());
  }
  filter.push_back(");;All files (*.*)");

  QString filename = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Input Model"), ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(),
                                                  filter, nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (!filename.isEmpty())
    lineEditSourceFilename->setText(filename);
}

void ezQtSceneImportDlg::on_OutputBrowse_clicked()
{
  QString filename = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Output Scene"), ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(),
                                                    "ezScene (*.ezScene)", nullptr, QFileDialog::Option::DontResolveSymlinks | QFileDialog::Option::DontConfirmOverwrite);

  if (!filename.isEmpty())
  {
    if (ezFileSystem::ExistsFile(filename.toUtf8().data()))
    {
      QMessageBox::warning(this, "Failed set output filename", "Can't overwrite existing files.");
      return;
    }
    lineEditOutputFilename->setText(filename);
  }
}
