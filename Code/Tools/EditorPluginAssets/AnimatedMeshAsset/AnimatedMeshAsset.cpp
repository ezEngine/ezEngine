#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter/Mesh.h>
#include <ModelImporter/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocument, 5, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

static ezMat3 CalculateTransformationMatrix(const ezAnimatedMeshAssetProperties* pProp)
{
  const float us = ezMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  return ezBasisAxis::CalculateTransformationMatrix(pProp->m_ForwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

ezAnimatedMeshAssetDocument::ezAnimatedMeshAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>(szDocumentPath, true)
{
}

ezStatus ezAnimatedMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                             const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimatedMeshAssetProperties* pProp = GetProperties();

  if (pProp->m_sSkeletonFile.IsEmpty())
    return ezStatus("No skeleton was specified for animated mesh asset");

  ezMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9);
  range.BeginNextStep("Importing Mesh");

  EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));

  range.BeginNextStep("Writing Result");
  desc.SetSkeleton(ezResourceManager::LoadResource<ezSkeletonResource>(pProp->m_sSkeletonFile)); // we actually only want the handle for serialization
  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimatedMeshAssetDocument::CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc)
{
  ezProgressRange range("Mesh Import", 6, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  const ezMat3 mTransformation = CalculateTransformationMatrix(pProp);

  ezSharedPtr<ezModelImporter::Scene> pScene;
  ezModelImporter::Mesh* pMesh = nullptr;
  EZ_SUCCEED_OR_RETURN(ezMeshImportUtils::TryImportMesh(pScene, pMesh, pProp->m_sMeshFile, "", mTransformation,
                                                        pProp->m_bRecalculateNormals, pProp->m_bInvertNormals, range, desc, true));

  range.BeginNextStep("Importing Materials");

  // Option material slot count correction & material import.
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != pMesh->GetNumSubMeshes())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

    ezMeshImportUtils::UpdateMaterialSlots(GetDocumentPath(), *pScene, *pMesh, pProp->m_bImportMaterials,
                                           pProp->m_bUseSubFolderForImportedMaterials, pProp->m_sMeshFile, pProp->m_Slots);

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  range.BeginNextStep("Setting Materials");

  ezMeshImportUtils::AddMeshToDescriptor(desc, *pScene, *pMesh, pProp->m_Slots);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimatedMeshAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezAnimatedMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimatedMeshAssetDocumentGenerator::ezAnimatedMeshAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
}

ezAnimatedMeshAssetDocumentGenerator::~ezAnimatedMeshAssetDocumentGenerator() {}

void ezAnimatedMeshAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath,
                                                          ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;
  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.WithMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.png";
  }

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport.NoMaterials";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.png";
  }
}

ezStatus ezAnimatedMeshAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info,
                                                        ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezAnimatedMeshAssetDocument* pAssetDoc = ezDynamicCast<ezAnimatedMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezAnimatedMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", szDataDirRelativePath);

  if (info.m_sName == "AnimatedMeshImport.WithMaterials")
  {
    accessor.SetValue("ImportMaterials", true);
  }

  if (info.m_sName == "AnimatedMeshImport.NoMaterials")
  {
    accessor.SetValue("ImportMaterials", false);
  }

  return ezStatus(EZ_SUCCESS);
}
