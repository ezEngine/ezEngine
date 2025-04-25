#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/Dialogs/MeshImportDlg.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

bool ezMeshAssetDocumentGenerator::s_bCreateMaterials = true;
bool ezMeshAssetDocumentGenerator::s_bUseSharedMaterials = false;
bool ezMeshAssetDocumentGenerator::s_bReuseSkeleton = false;
bool ezMeshAssetDocumentGenerator::s_bImportAllClips = false;
ezUuid ezMeshAssetDocumentGenerator::s_SharedSkeleton;

ezMeshAssetDocumentGenerator::ezMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
  AddSupportedFileType("vox");
}

ezMeshAssetDocumentGenerator::ezMeshAssetDocumentGenerator(bool bAnimMesh)
{
  m_bAnimatedMesh = bAnimMesh;
}

ezMeshAssetDocumentGenerator::~ezMeshAssetDocumentGenerator() = default;

void ezMeshAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "MeshImport";
    info.m_sIcon = ":/AssetIcons/Mesh.svg";
  }
}

ezStatus ezMeshAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping mesh import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  ezStringBuilder sSharedMaterialsFolderAbs = pPref->m_sSharedMaterialFolder;

  if (sSharedMaterialsFolderAbs.IsEmpty())
  {
    ezStringBuilder tmp = ezToolsProject::GetSingleton()->GetProjectDirectory();
    tmp.AppendPath("Materials");

    sSharedMaterialsFolderAbs = tmp;
  }

  if (m_bShowImportDlg)
  {
    ezMeshImportDlg dlg(nullptr);
    dlg.m_bShowAnimMeshOptions = m_bAnimatedMesh;
    dlg.m_sTitle = sInputFileRel;
    dlg.m_bCreateMaterials = s_bCreateMaterials;
    dlg.m_sSharedMaterialsFolderAbs = sSharedMaterialsFolderAbs;
    dlg.m_bUseSharedMaterials = s_bUseSharedMaterials;
    dlg.m_bReuseExistingSkeleton = s_bReuseSkeleton;
    dlg.m_SharedSkeleton = s_SharedSkeleton;
    dlg.m_bImportAnimationClips = s_bImportAllClips;

    if (dlg.exec() != QDialog::Accepted)
    {
      return ezStatus("User aborted asset import.");
    }

    s_bCreateMaterials = dlg.m_bCreateMaterials;

    if (s_bCreateMaterials)
    {
      s_bUseSharedMaterials = dlg.m_bUseSharedMaterials;

      if (s_bUseSharedMaterials)
      {
        sSharedMaterialsFolderAbs = dlg.m_sSharedMaterialsFolderAbs;
        pPref->m_sSharedMaterialFolder = dlg.m_sSharedMaterialsFolderAbs;
      }
    }

    if (m_bAnimatedMesh)
    {
      s_bReuseSkeleton = dlg.m_bReuseExistingSkeleton;
      s_SharedSkeleton = dlg.m_SharedSkeleton;
      s_bImportAllClips = dlg.m_bImportAnimationClips;
    }

    if (dlg.m_bApplyToAll)
    {
      m_bShowImportDlg = false;
    }
  }

  ezStringBuilder sMaterialFolder = sInputFileAbs;

  if (s_bUseSharedMaterials)
  {
    sMaterialFolder = sSharedMaterialsFolderAbs;
  }

  ezHybridArray<ezMaterialResourceSlot, 8> materials;
  ezUniquePtr<ezModelImporter2::Importer> pImporter;

  if (s_bCreateMaterials || (m_bAnimatedMesh && s_bImportAllClips))
  {
    pImporter = ezModelImporter2::RequestImporterForFileType(sInputFileAbs);
    if (pImporter == nullptr)
      return ezStatus("No known importer for this file type.");

    ezMeshResourceDescriptor desc;

    ezModelImporter2::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;
    opt.m_pMeshOutput = &desc;

    if (pImporter->Import(opt).Failed())
      return ezStatus("Model importer was unable to read this asset.");

    ezMeshImportUtils::SetMeshAssetMaterialSlots(materials, pImporter.Borrow());
    ezMeshImportUtils::ImportMeshAssetMaterials(materials, sMaterialFolder, pImporter.Borrow());
  }

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  return ConfigureMeshDocument(pDoc, sInputFileRel, sOutFile, pImporter.Borrow(), materials, out_generatedDocuments);
}

ezStatus ezMeshAssetDocumentGenerator::ConfigureMeshDocument(ezDocument* pDoc, ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezMeshAssetDocument* pAssetDoc = ezDynamicCast<ezMeshAssetDocument*>(pDoc);

  auto pPropObj = pAssetDoc->GetPropertyObject();

  ezObjectCommandAccessor ca(pAssetDoc->GetCommandHistory());
  ca.StartTransaction("Init Values");
  ca.SetValueByName(pPropObj, "MeshFile", sInputFile).AssertSuccess();
  ca.SetValueByName(pPropObj, "ImportMaterials", false).AssertSuccess();

  for (ezUInt32 i = 0; i < materials.GetCount(); ++i)
  {
    ezUuid guid = ezUuid::MakeUuid();
    ca.AddObjectByName(pPropObj, "Materials", i, ezGetStaticRTTI<ezMaterialResourceSlot>(), guid).AssertSuccess();

    auto* pChildMatObj = ca.GetObject(guid);
    ca.SetValueByName(pChildMatObj, "Label", materials[i].m_sLabel).AssertSuccess();
    ca.SetValueByName(pChildMatObj, "Resource", materials[i].m_sResource).AssertSuccess();
  }

  ca.FinishTransaction();

  ezLog::Success("Imported mesh: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezAnimatedMeshAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimatedMeshAssetDocumentGenerator::ezAnimatedMeshAssetDocumentGenerator()
  : ezMeshAssetDocumentGenerator(true)
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

ezAnimatedMeshAssetDocumentGenerator::~ezAnimatedMeshAssetDocumentGenerator() = default;

void ezAnimatedMeshAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "AnimatedMeshImport";
    info.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  }
}

ezStatus ezAnimatedMeshAssetDocumentGenerator::ConfigureMeshDocument(ezDocument* pMainDoc, ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  ezUuid skeletonGuid = s_SharedSkeleton;

  // create skeleton asset
  if (!s_bReuseSkeleton)
  {
    ezStringBuilder sOutFile2;

    sOutFile2 = sOutFile;
    sOutFile2.ChangeFileExtension("ezSkeletonAsset");

    if (ezOSFile::ExistsFile(sOutFile2))
    {
      ezLog::Info("Skipping skeleton import, file has been imported before: '{}'", sOutFile2);

      auto pSkeletonDoc = ezAssetCurator::GetSingleton()->FindSubAsset(sOutFile2);
      skeletonGuid = pSkeletonDoc->m_Data.m_Guid;
    }
    else
    {
      ezDocument* pSkelDoc = pApp->CreateDocument(sOutFile2, ezDocumentFlags::None);
      if (pSkelDoc == nullptr)
        return ezStatus("Could not create skeleton document");

      ezStringBuilder sAnimMeshGuid;
      ezConversionUtils::ToString(pMainDoc->GetGuid(), sAnimMeshGuid);

      out_generatedDocuments.PushBack(pSkelDoc);

      ezSkeletonAssetDocument* pSkeletonDoc = ezDynamicCast<ezSkeletonAssetDocument*>(pSkelDoc);

      auto pSkeletonPropObj = pSkeletonDoc->GetPropertyObject();

      ezObjectCommandAccessor ca(pSkeletonDoc->GetCommandHistory());
      ca.StartTransaction("Init Values");
      ca.SetValueByName(pSkeletonPropObj, "File", sInputFile).AssertSuccess();
      ca.SetValueByName(pSkeletonPropObj, "PreviewMesh", sAnimMeshGuid.GetView()).AssertSuccess();
      ca.FinishTransaction();

      skeletonGuid = pSkeletonDoc->GetGuid();

      ezLog::Success("Imported skeleton: '{}'", sOutFile2);
    }
  }

  // configure animated mesh asset
  {
    ezAnimatedMeshAssetDocument* pAnimMeshDoc = ezDynamicCast<ezAnimatedMeshAssetDocument*>(pMainDoc);

    auto pPropObj = pAnimMeshDoc->GetPropertyObject();

    ezStringBuilder sSkeletonGuid;
    ezConversionUtils::ToString(skeletonGuid, sSkeletonGuid);

    ezObjectCommandAccessor ca(pAnimMeshDoc->GetCommandHistory());
    ca.StartTransaction("Init Values");
    ca.SetValueByName(pPropObj, "MeshFile", sInputFile).AssertSuccess();
    ca.SetValueByName(pPropObj, "ImportMaterials", false).AssertSuccess();
    ca.SetValueByName(pPropObj, "DefaultSkeleton", sSkeletonGuid.GetView()).AssertSuccess();

    for (ezUInt32 i = 0; i < materials.GetCount(); ++i)
    {
      ezUuid guid = ezUuid::MakeUuid();
      ca.AddObjectByName(pPropObj, "Materials", i, ezGetStaticRTTI<ezMaterialResourceSlot>(), guid).AssertSuccess();

      auto* pChildMatObj = ca.GetObject(guid);
      ca.SetValueByName(pChildMatObj, "Label", materials[i].m_sLabel).AssertSuccess();
      ca.SetValueByName(pChildMatObj, "Resource", materials[i].m_sResource).AssertSuccess();
    }

    ca.FinishTransaction();

    ezLog::Success("Imported animated mesh: '{}'", sOutFile);
  }

  // create animation clip assets
  if (s_bImportAllClips)
  {
    ezStringBuilder sFilename;
    ezStringBuilder sOutFile2;

    ezStringBuilder sPreviewMesh;
    ezConversionUtils::ToString(pMainDoc->GetGuid(), sPreviewMesh);

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      sFilename = clip;
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);
      sOutFile2.ChangeFileExtension("ezAnimationClipAsset");

      if (ezOSFile::ExistsFile(sOutFile2))
      {
        ezLog::Info("Skipping animation clip import, file has been imported before: '{}'", sOutFile2);
        continue;
      }

      ezDocument* pAnimDoc = pApp->CreateDocument(sOutFile2, ezDocumentFlags::None);
      if (pAnimDoc == nullptr)
        return ezStatus("Could not create animation clip document");

      out_generatedDocuments.PushBack(pAnimDoc);

      ezAnimationClipAssetDocument* pAnimClipDoc = ezDynamicCast<ezAnimationClipAssetDocument*>(pAnimDoc);

      auto pAnimPropObj = pAnimClipDoc->GetPropertyObject();

      ezObjectCommandAccessor ca(pAnimClipDoc->GetCommandHistory());
      ca.StartTransaction("Init Values");

      ca.SetValueByName(pAnimPropObj, "File", sInputFile).AssertSuccess();
      ca.SetValueByName(pAnimPropObj, "UseAnimationClip", clip).AssertSuccess();
      ca.SetValueByName(pAnimPropObj, "PreviewMesh", sPreviewMesh.GetView()).AssertSuccess();

      ca.FinishTransaction();

      ezLog::Success("Imported animation clip: '{}'", sOutFile2);
    }
  }

  return ezStatus(EZ_SUCCESS);
}
