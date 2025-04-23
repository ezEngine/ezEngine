#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocument, 8, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimatedMeshAssetDocument::ezAnimatedMeshAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezAnimatedMeshAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezTransformStatus ezAnimatedMeshAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezProgressRange range("Transforming Asset", 2, false);

  ezAnimatedMeshAssetProperties* pProp = GetProperties();

  if (pProp->m_sDefaultSkeleton.IsEmpty())
  {
    return ezStatus("Animated mesh doesn't have a default skeleton assigned.");
  }

  ezMeshResourceDescriptor desc;

  range.SetStepWeighting(0, 0.9f);
  range.BeginNextStep("Importing Mesh");

  EZ_SUCCEED_OR_RETURN(CreateMeshFromFile(pProp, desc));

  // the properties object can get invalidated by the CreateMeshFromFile() call
  pProp = GetProperties();

  range.BeginNextStep("Writing Result");

  if (!pProp->m_sDefaultSkeleton.IsEmpty())
  {
    desc.m_hDefaultSkeleton = ezResourceManager::LoadResource<ezSkeletonResource>(pProp->m_sDefaultSkeleton);
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAnimatedMeshAssetDocument::CreateMeshFromFile(ezAnimatedMeshAssetProperties* pProp, ezMeshResourceDescriptor& desc)
{
  ezProgressRange range("Mesh Import", 5, false);

  range.SetStepWeighting(0, 0.7f);
  range.BeginNextStep("Importing Mesh Data");

  ezStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return ezStatus(ezFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  ezUniquePtr<ezModelImporter2::Importer> pImporter = ezModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return ezStatus("No known importer for this file type.");

  ezModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_bImportSkinningData = true;
  opt.m_bRecomputeNormals = pProp->m_bRecalculateNormals;
  opt.m_bRecomputeTangents = pProp->m_bRecalculateTrangents;
  opt.m_pMeshOutput = &desc;
  opt.m_MeshNormalsPrecision = pProp->m_NormalPrecision;
  opt.m_MeshTexCoordsPrecision = pProp->m_TexCoordPrecision;
  opt.m_MeshVertexColorConversion = pProp->m_VertexColorConversion;
  opt.m_MeshBoneWeightPrecision = pProp->m_BoneWeightPrecision;
  opt.m_bNormalizeWeights = pProp->m_bNormalizeWeights;
  // opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pProp->m_bSimplifyMesh)
  {
    opt.m_uiMeshSimplification = pProp->m_uiMeshSimplification;
    opt.m_uiMaxSimplificationError = pProp->m_uiMaxSimplificationError;
    opt.m_bAggressiveSimplification = pProp->m_bAggressiveSimplification;
  }

  if (pImporter->Import(opt).Failed())
    return ezStatus("Model importer was unable to read this asset.");

  range.BeginNextStep("Importing Materials");

  // correct the number of material slots
  if (pProp->m_bImportMaterials || pProp->m_Slots.GetCount() != desc.GetSubMeshes().GetCount())
  {
    GetObjectAccessor()->StartTransaction("Update Mesh Materials");

    ezMeshImportUtils::SetMeshAssetMaterialSlots(pProp->m_Slots, pImporter.Borrow());

    if (pProp->m_bImportMaterials)
    {
      ezMeshImportUtils::ImportMeshAssetMaterials(pProp->m_Slots, GetDocumentPath(), pImporter.Borrow());
    }

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();

    // Need to reacquire pProp pointer since it might be reallocated.
    pProp = GetProperties();
  }

  ezMeshImportUtils::CopyMeshAssetMaterialSlotToResource(desc, pProp->m_Slots);

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezAnimatedMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
