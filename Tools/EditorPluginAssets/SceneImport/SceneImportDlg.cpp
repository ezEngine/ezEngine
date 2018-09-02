#include <PCH.h>

#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/SceneImport/SceneImportDlg.moc.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <ModelImporter/Node.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

ezQtSceneImportDlg::ezQtSceneImportDlg(QWidget* parent)
    : QDialog(parent)
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

namespace
{
  struct RecursiveMeshImport
  {
    RecursiveMeshImport(ezObjectAccessorBase& sceneAccessor, const ezDocumentObjectManager& objectManager,
                        const ezModelImporter::Scene& scene, const ezHashTable<const ezModelImporter::Mesh*, ezString>& rawMeshToFile)
        : sceneAccessor(sceneAccessor)
        , objectManager(objectManager)
        , scene(scene)
        , rawMeshToFile(rawMeshToFile)
    {
    }

    ezObjectAccessorBase& sceneAccessor;
    const ezDocumentObjectManager& objectManager;
    const ezModelImporter::Scene& scene;
    const ezHashTable<const ezModelImporter::Mesh*, ezString>& rawMeshToFile;

    void ImportNodeRecursive(const ezModelImporter::HierarchyObject* object, const ezDocumentObject* parentNode)
    {
      // Create a ezGameObject if this is a node or something else with parentNode beeing null.
      const ezDocumentObject* currentGameObject;
      if (object->m_Type == ezModelImporter::ObjectHandle::NODE || !parentNode)
      {
        ezUuid newGameObjectId;
        newGameObjectId.CreateNewUuid();
        if (parentNode)
          sceneAccessor.AddObject(parentNode, "Children", -1, ezRTTI::FindTypeByName("ezGameObject"), newGameObjectId).LogFailure();
        else
          sceneAccessor
              .AddObject(nullptr, static_cast<ezAbstractProperty*>(nullptr), -1, ezRTTI::FindTypeByName("ezGameObject"), newGameObjectId)
              .LogFailure();
        currentGameObject = objectManager.GetObject(newGameObjectId);
      }
      else
      {
        currentGameObject = parentNode;
      }

      // Set Name.
      sceneAccessor.SetValue(currentGameObject, "Name", object->m_Name);

      // Setup rest...
      switch (object->m_Type)
      {
        case ezModelImporter::ObjectHandle::NODE:
        {
          const ezModelImporter::Node* node = object->Cast<ezModelImporter::Node>();

          sceneAccessor.SetValue(currentGameObject, "LocalPosition", node->m_RelativeTransform.m_vPosition);
          sceneAccessor.SetValue(currentGameObject, "LocalRotation", node->m_RelativeTransform.m_qRotation);
          sceneAccessor.SetValue(currentGameObject, "LocalScaling", node->m_RelativeTransform.m_vScale);

          for (ezModelImporter::ObjectHandle childHandle : node->m_Children)
          {
            ImportNodeRecursive(scene.GetObject(childHandle), currentGameObject);
          }
        }
        break;

        case ezModelImporter::ObjectHandle::MESH:
        {
          EZ_ASSERT_DEBUG(currentGameObject, "No node to attach a parent component to.");
          const ezString* meshId = nullptr;
          if (rawMeshToFile.TryGetValue(object->Cast<ezModelImporter::Mesh>(), meshId))
          {
            ezUuid newMeshComponentId;
            sceneAccessor.AddObject(currentGameObject, "Components", -1, ezRTTI::FindTypeByName("ezMeshComponent"), newMeshComponentId)
                .LogFailure();
            const ezDocumentObject* newMeshComponent = objectManager.GetObject(newMeshComponentId);
            sceneAccessor.SetValue(newMeshComponent, "Mesh", *meshId);
          }
          else
          {
            // Silently ignore, error should have already been handled.
          }
        }
        break;
      }

      // "LocalPosition";


      // Call for children.
    }
  };
}

void ezQtSceneImportDlg::on_accepted()
{
  ezString inputFilename = lineEditSourceFilename->text().toUtf8().data();
  ezString outputFilename = lineEditOutputFilename->text().toUtf8().data();


  // Clear cache when we are done here.
  EZ_SCOPE_EXIT(ezModelImporter::Importer::GetSingleton()->ClearCachedScenes());

  // Load model.
  ezSharedPtr<ezModelImporter::Scene> rawScene =
      ezModelImporter::Importer::GetSingleton()->ImportScene(inputFilename, ezModelImporter::ImportFlags::All, true);
  if (!rawScene)
  {
    QMessageBox::critical(this, "Failed to load import", "Failed to load the input model file. Details can be found in the editor log.");
    return;
  }

  ezStringBuilder tmp;

  // Create meshes.
  ezHashTable<const ezModelImporter::Mesh*, ezString> rawMeshToFile;
  rawMeshToFile.Reserve(rawScene->GetMeshes().GetCount());

  ezStringBuilder meshDirectory = outputFilename;
  meshDirectory = meshDirectory.GetFileDirectory();
  meshDirectory.AppendPath("Meshes");
  for (auto meshIt = rawScene->GetMeshes().GetIterator(); meshIt.IsValid(); ++meshIt)
  {
    // Figure out a good filename.
    ezStringBuilder validFileMeshName;
    if (ezPathUtils::MakeValidFilename(meshIt.Value()->m_Name, '_', validFileMeshName).Failed())
    {
      ezLog::Error("Failed to create a filename for mesh '{0}'", meshIt.Value()->m_Name);
      continue;
    }
    ezStringBuilder meshFilename = meshDirectory;
    meshFilename.AppendPath(validFileMeshName);
    meshFilename.Append(".ezMeshAsset");

    {
      const auto assetInfo = ezAssetCurator::GetSingleton()->FindSubAsset(meshFilename);
      if (assetInfo != nullptr)
      {
        rawMeshToFile.Insert(meshIt.Value().Borrow(), ezConversionUtils::ToString(assetInfo->m_pAssetInfo->m_Info->m_DocumentID, tmp));
        continue;
      }
    }

    // Create document.
    ezMeshAssetDocument* meshDocument =
        ezDynamicCast<ezMeshAssetDocument*>(ezQtEditorApp::GetSingleton()->CreateDocument(meshFilename, ezDocumentFlags::None));
    if (!meshDocument)
    {
      ezLog::Error("Failed to create a document for mesh '{0}' in '{1}'", meshIt.Value()->m_Name, meshFilename);
      continue;
    }

    // Set mesh.
    {
      ezObjectAccessorBase* pAccessor = meshDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Set Mesh and Submesh (Scene Import)");

      ezDocumentObject* pMeshAsset = meshDocument->GetPropertyObject();
      pAccessor->SetValue(pMeshAsset, "MeshFile", inputFilename.GetData()).LogFailure();
      pAccessor->SetValue(pMeshAsset, "SubmeshName", meshIt.Value()->m_Name).LogFailure();
      pAccessor->SetValue(pMeshAsset, "UseSubfolderForMaterialImport", false).LogFailure();

      pAccessor->FinishTransaction();
    }

    // Transform.
    {
      meshDocument->SaveDocument();
      ezAssetCurator::GetSingleton()->TransformAsset(meshDocument->GetGuid(), false);
      meshDocument->SaveDocument(); // Save again since transform may change properties!

      rawMeshToFile.Insert(meshIt.Value().Borrow(), ezConversionUtils::ToString(meshDocument->GetGuid(), tmp));

      meshDocument->GetDocumentManager()->CloseDocument(meshDocument);
    }
  }

  // Create scene.
  ezDocument* sceneDocument = ezQtEditorApp::GetSingleton()->CreateDocument(outputFilename, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList);
  if (!sceneDocument)
  {
    QMessageBox::critical(this, "Failed to load import", "Failed to create scene document.");
    return;
  }

  // Create node hierachy.
  {
    ezObjectAccessorBase* pAccessor = sceneDocument->GetObjectAccessor();
    pAccessor->StartTransaction("Setting up Node Hierarchy (Scene Import)");

    RecursiveMeshImport import(*pAccessor, *sceneDocument->GetObjectManager(), *rawScene, rawMeshToFile);

    for (const ezModelImporter::HierarchyObject* rootObject : rawScene->GetRootObjects())
    {
      import.ImportNodeRecursive(rootObject, nullptr);
    }

    pAccessor->FinishTransaction();
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

  QString filename = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Input Model"),
                                                  ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(), filter, nullptr,
                                                  QFileDialog::Option::DontResolveSymlinks);

  if (!filename.isEmpty())
    lineEditSourceFilename->setText(filename);
}

void ezQtSceneImportDlg::on_OutputBrowse_clicked()
{
  QString filename = QFileDialog::getSaveFileName(
      QApplication::activeWindow(), QLatin1String("Output Scene"), ezToolsProject::GetSingleton()->GetProjectDirectory().GetData(),
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
