#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/Dialogs/MeshImportDlg.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/Containers/ArrayMap.h>
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
bool ezMeshAssetDocumentGenerator::s_bAddLODs = false;
ezUInt8 ezMeshAssetDocumentGenerator::s_uiNumLODs = 1;
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
    dlg.m_bAddLODs = s_bAddLODs;
    dlg.m_uiNumLODs = s_uiNumLODs;
    dlg.m_sMeshLodPrefix = pPref->m_sMeshLodPrefix.IsEmpty() ? ezString("$LOD") : pPref->m_sMeshLodPrefix;

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

    s_bAddLODs = dlg.m_bAddLODs;
    if (s_bAddLODs)
    {
      s_uiNumLODs = ezMath::Clamp<ezUInt8>(dlg.m_uiNumLODs, 0, 4);
      pPref->m_sMeshLodPrefix = dlg.m_sMeshLodPrefix;
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

  return ConfigureMeshDocument(sInputFileRel, sOutFile, pImporter.Borrow(), materials, out_generatedDocuments);
}

static void FindLODs(ezArrayMap<ezUInt32, ezString>& out_foundLods, ezModelImporter2::Importer* pImporter)
{
  out_foundLods.Clear();

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  if (!pPref->m_sMeshLodPrefix.IsEmpty())
  {
    ezStringBuilder lod;

    for (ezUInt32 i = 0; i < 4; ++i)
    {
      lod = pPref->m_sMeshLodPrefix;
      lod.AppendFormat("{}", i);

      for (const auto& meshName : pImporter->m_OutputMeshNames)
      {
        if (meshName.StartsWith_NoCase(lod) || meshName.EndsWith_NoCase(lod))
        {
          out_foundLods[i] = lod;
        }
      }
    }
  }

  out_foundLods.Sort();
}

static void SetMeshLod(ezUInt32 uiLod, ezArrayMap<ezUInt32, ezString>& ref_foundLods, ezDocumentObject* pPropObj, ezObjectCommandAccessor& ref_accessor)
{
  if (uiLod > 0)
  {
    if (ref_foundLods.IsEmpty())
    {
      ref_accessor.SetValueByName(pPropObj, "SimplifyMesh", true).AssertSuccess();

      const ezInt32 uiSimp[5] = {0, 50, 75, 90, 95};
      const ezInt32 uiErro[5] = {0, 5, 5, 10, 15};
      ref_accessor.SetValueByName(pPropObj, "MeshSimplification", uiSimp[uiLod]).AssertSuccess();
      ref_accessor.SetValueByName(pPropObj, "MaxSimplificationError", uiErro[uiLod]).AssertSuccess();
    }
    else
    {
      // always use the smallest next LOD that was found
      const ezString& sLodToUse = ref_foundLods.GetValue(0);

      ref_accessor.SetValueByName(pPropObj, "MeshIncludeTags", sLodToUse).AssertSuccess();

      ref_foundLods.RemoveAtAndCopy(0);
    }
  }
}

ezStatus ezMeshAssetDocumentGenerator::ConfigureMeshDocument(ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  ezUInt32 uiNumLODs = 1;

  ezArrayMap<ezUInt32, ezString> foundLods;
  if (s_bAddLODs)
  {
    FindLODs(foundLods, pImporter);
    uiNumLODs += (!foundLods.IsEmpty()) ? foundLods.GetCount() : s_uiNumLODs;
  }

  ezStringBuilder sFinalName, sFinalPath;

  for (ezUInt32 uiLod = 0; uiLod < uiNumLODs; ++uiLod)
  {
    sFinalPath = sOutFile;

    if (uiLod > 0)
    {
      // sFinalName.SetFormat("{}_lod{}", sOutFile.GetFileName(), uiLod);
      sFinalName.SetFormat("{}_data/LOD-{}", sOutFile.GetFileName(), uiLod);
      sFinalPath.ChangeFileName(sFinalName);
    }

    ezDocument* pDoc = pApp->CreateDocument(sFinalPath, ezDocumentFlags::None);
    if (pDoc == nullptr)
      return ezStatus(ezFmt("Could not create document '{}'", sFinalPath));

    out_generatedDocuments.PushBack(pDoc);

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

    SetMeshLod(uiLod, foundLods, pPropObj, ca);

    ca.FinishTransaction();

    ezLog::Success("Imported mesh: '{}'", sFinalPath);
  }

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

ezStatus ezAnimatedMeshAssetDocumentGenerator::ConfigureMeshDocument(ezStringView sInputFile, ezStringView sOutFile, ezModelImporter2::Importer* pImporter, ezArrayPtr<ezMaterialResourceSlot> materials, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  ezDocument* pMainDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pMainDoc == nullptr)
    return ezStatus(ezFmt("Could not create document '{}'", sOutFile));

  out_generatedDocuments.PushBack(pMainDoc);

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
    ezStringBuilder sFinalName, sFinalPath;

    ezUInt32 uiNumLODs = 1;

    ezArrayMap<ezUInt32, ezString> foundLods;
    if (s_bAddLODs)
    {
      FindLODs(foundLods, pImporter);
      uiNumLODs += (!foundLods.IsEmpty()) ? foundLods.GetCount() : s_uiNumLODs;
    }

    for (ezUInt32 uiLod = 0; uiLod < uiNumLODs; ++uiLod)
    {
      sFinalPath = sOutFile;

      ezAnimatedMeshAssetDocument* pAnimMeshDoc;

      if (uiLod == 0)
      {
        pAnimMeshDoc = ezDynamicCast<ezAnimatedMeshAssetDocument*>(pMainDoc);
      }
      else
      {
        if (uiLod > 0)
        {
          // sFinalName.SetFormat("{}_lod{}", sOutFile.GetFileName(), uiLod);
          sFinalName.SetFormat("{}_data/LOD-{}", sOutFile.GetFileName(), uiLod);
          sFinalPath.ChangeFileName(sFinalName);
        }

        pAnimMeshDoc = ezDynamicCast<ezAnimatedMeshAssetDocument*>(pApp->CreateDocument(sFinalPath, ezDocumentFlags::None));
        if (pAnimMeshDoc == nullptr)
          return ezStatus(ezFmt("Could not create document '{}'", sFinalPath));

        out_generatedDocuments.PushBack(pAnimMeshDoc);
      }

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

      SetMeshLod(uiLod, foundLods, pPropObj, ca);

      ca.FinishTransaction();

      ezLog::Success("Imported animated mesh: '{}'", sFinalPath);
    }
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
      ezPathUtils::MakeValidFilename(clip, '-', sFilename);
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
