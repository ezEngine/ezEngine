#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/Util/MeshLodCreator.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <EditorTest/MeshLod/MeshLodTest.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Declarations.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorMeshLodTest s_EditorMeshLodTest;

const char* ezEditorMeshLodTest::GetTestName() const
{
  return "Mesh LOD Tests";
}

void ezEditorMeshLodTest::SetupSubTests()
{
  AddSubTest("Simplification Ladder", SubTests::ST_SimplificationLadder);
  AddSubTest("Create LODs", SubTests::ST_CreateLods);
  AddSubTest("Continues From Simplified Base", SubTests::ST_ContinuesFromSimplifiedBase);
  AddSubTest("Transferred Settings", SubTests::ST_TransferredSettings);
  AddSubTest("Existing LODs", SubTests::ST_ExistingLods);
  AddSubTest("Prefab Picks Them Up", SubTests::ST_PrefabPicksThemUp);
  AddSubTest("Primitive Mesh", SubTests::ST_PrimitiveMesh);
  AddSubTest("Multiple Meshes", SubTests::ST_MultipleMeshes);
  AddSubTest("Sub Mesh Variants Do Not Share", SubTests::ST_SubMeshVariantsDoNotShare);
}

ezResult ezEditorMeshLodTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  // the project has to be settled first, or the curator keeps rehashing while the sub-tests
  // create their assets
  if (ezStatus res = ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::TriggeredManually); res.Failed())
  {
    ezLog::Error("Asset transform failed: {}", res.GetMessageString());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezEditorMeshLodTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorMeshLodTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_SimplificationLadder:
      SimplificationLadder();
      break;
    case SubTests::ST_CreateLods:
      CreateLods();
      break;
    case SubTests::ST_ContinuesFromSimplifiedBase:
      ContinuesFromSimplifiedBase();
      break;
    case SubTests::ST_TransferredSettings:
      TransferredSettings();
      break;
    case SubTests::ST_ExistingLods:
      ExistingLods();
      break;
    case SubTests::ST_PrefabPicksThemUp:
      PrefabPicksThemUp();
      break;
    case SubTests::ST_PrimitiveMesh:
      PrimitiveMesh();
      break;
    case SubTests::ST_SubMeshVariantsDoNotShare:
      SubMeshVariantsDoNotShare();
      break;

    case SubTests::ST_MultipleMeshes:
      MultipleMeshes();
      break;
  }

  return ezTestAppRun::Quit;
}

ezString ezEditorMeshLodTest::MakePrivateSourceMesh(const char* szName)
{
  ezStringBuilder sSrc = m_sProjectPath;
  sSrc.AppendPath("Meshes/Cube.obj");

  ezStringBuilder sRelative;
  sRelative.SetFormat("Meshes/{}.obj", szName);

  ezStringBuilder sDst = m_sProjectPath;
  sDst.AppendPath(sRelative);

  if (ezOSFile::CopyFile(sSrc, sDst).Failed())
    return {};

  ezFileSystemModel::GetSingleton()->NotifyOfChange(sDst);
  return sRelative;
}

ezUuid ezEditorMeshLodTest::CreateMeshAsset(const char* szRelativePath, const char* szSourceFile, const ezVariantDictionary* pExtraProperties)
{
  ezStringBuilder sPath = m_sProjectPath;
  sPath.AppendPath(szRelativePath);

  ezDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sPath, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return {};

  {
    ezDocumentObject* pProps = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();

    pAcc->StartTransaction("Init");
    pAcc->SetValueByName(pProps, "MeshFile", szSourceFile).AssertSuccess();

    if (pExtraProperties != nullptr)
    {
      for (auto it : *pExtraProperties)
      {
        pAcc->SetValueByName(pProps, it.Key(), it.Value()).AssertSuccess();
      }
    }

    pAcc->FinishTransaction();
  }

  pDoc->SaveDocument(true).AssertSuccess();

  const ezUuid guid = pDoc->GetGuid();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  return guid;
}

ezVariant ezEditorMeshLodTest::ReadAssetProperty(ezStringView sAbsPath, ezStringView sProperty)
{
  ezDocument* pDoc = m_pApplication->m_pEditorApp->OpenDocument(sAbsPath, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return {};

  EZ_SCOPE_EXIT(pDoc->GetDocumentManager()->CloseDocument(pDoc));

  const auto& children = pDoc->GetObjectManager()->GetRootObject()->GetChildren();
  if (children.GetCount() != 1)
    return {};

  return children[0]->GetTypeAccessor().GetValue(sProperty);
}

void ezEditorMeshLodTest::SimplificationLadder()
{
  // Each level takes the midpoint of what is left between the level before it and 100, so from an
  // unsimplified mesh the ladder is 50, 75, 88, 94.
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(0, 1), 50);
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(0, 2), 75);
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(0, 3), 88);
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(0, 4), 94);

  // A mesh that is already simplified continues from where it stands: LOD-1 of a mesh at 50% has to
  // be sparser than the mesh, not equal to it.
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(50, 1), 75);
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(50, 2), 88);
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplification(75, 1), 88);

  // every level has to be strictly sparser than the one before, from every starting point
  for (ezUInt8 uiBase = 0; uiBase < 99; ++uiBase)
  {
    ezUInt8 uiPrev = uiBase;

    for (ezUInt32 uiLod = 1; uiLod <= ezMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      const ezUInt8 uiThis = ezMeshLodCreator::GetLodSimplification(uiBase, uiLod);

      // At the very top the steps round down to nothing, which is expected - there is no room left.
      if (uiPrev >= 97)
        break;

      EZ_TEST_BOOL_MSG(uiThis > uiPrev, "Every LOD has to be sparser than the one before it.");
      uiPrev = uiThis;
    }
  }

  // 100 would remove the entire mesh, so the ladder must never reach it.
  for (ezUInt8 uiBase = 0; uiBase <= 99; ++uiBase)
  {
    for (ezUInt32 uiLod = 1; uiLod <= ezMeshLodCreator::s_uiMaxLods; ++uiLod)
    {
      EZ_TEST_BOOL(ezMeshLodCreator::GetLodSimplification(uiBase, uiLod) < 100);
      EZ_TEST_BOOL(ezMeshLodCreator::GetLodSimplification(uiBase, uiLod) >= 1);
    }
  }

  // The tolerated error grows with the level, because a distant mesh can afford a coarser silhouette.
  EZ_TEST_INT(ezMeshLodCreator::GetLodSimplificationError(1), 5);
  EZ_TEST_BOOL(ezMeshLodCreator::GetLodSimplificationError(3) > ezMeshLodCreator::GetLodSimplificationError(1));

  // past the table it must still answer rather than reading out of bounds
  EZ_TEST_BOOL(ezMeshLodCreator::GetLodSimplificationError(ezMeshLodCreator::s_uiMaxLods) > 0);
}

void ezEditorMeshLodTest::CreateLods()
{
  const ezString sSource = MakePrivateSourceMesh("LodBasic");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshLod/Basic.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshLodSource source;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_BOOL(!source.m_bIsPrimitive);
  EZ_TEST_STRING(source.m_sMeshFile, sSource);
  EZ_TEST_INT_MSG(source.m_uiBaseSimplification, 0, "This mesh does not simplify, so the ladder starts from the full model.");

  // The folder name is what the prefab tool looks for, so it is not a free choice.
  EZ_TEST_BOOL_MSG(source.m_sLodFolder.EndsWith("Basic_data"), "The LODs belong in <MeshName>_data.");

  ezMeshLodOptions options;
  options.m_uiLodCount = 2;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  EZ_TEST_INT(uiCreated, 2);
  EZ_TEST_INT(uiSkipped, 0);

  const ezString sLod1 = ezMeshLodCreator::GetLodPath(source, 1);
  const ezString sLod2 = ezMeshLodCreator::GetLodPath(source, 2);

  if (!EZ_TEST_BOOL(ezOSFile::ExistsFile(sLod1) && ezOSFile::ExistsFile(sLod2)))
    return;

  // The exact names matter: LOD-1 and LOD-2, so that the prefab tool finds them.
  EZ_TEST_STRING(ezPathUtils::GetFileName(sLod1), "LOD-1");
  EZ_TEST_STRING(ezPathUtils::GetFileName(sLod2), "LOD-2");

  // each LOD is a simplified version of the same model, not a copy of it
  EZ_TEST_STRING(ReadAssetProperty(sLod1, "MeshFile").ConvertTo<ezString>(), sSource);
  EZ_TEST_BOOL(ReadAssetProperty(sLod1, "SimplifyMesh").ConvertTo<bool>() == true);
  EZ_TEST_INT(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<ezInt64>(), 50);
  EZ_TEST_INT(ReadAssetProperty(sLod2, "MeshSimplification").ConvertTo<ezInt64>(), 75);

  // and it must not re-import the materials, which would produce a second set for the same model
  EZ_TEST_BOOL(ReadAssetProperty(sLod1, "ImportMaterials").ConvertTo<bool>() == false);
}

void ezEditorMeshLodTest::ContinuesFromSimplifiedBase()
{
  const ezString sSource = MakePrivateSourceMesh("LodContinue");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // The mesh is already simplified, so its LODs have to go further rather than starting at 50 again.
  ezVariantDictionary extras;
  extras.Insert("SimplifyMesh", true);
  extras.Insert("MeshSimplification", (ezInt64)50);

  const ezUuid meshGuid = CreateMeshAsset("MeshLod/Continue.ezMeshAsset", sSource, &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshLodSource source;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_INT(source.m_uiBaseSimplification, 50);

  ezMeshLodOptions options;
  options.m_uiLodCount = 2;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  const ezString sLod1 = ezMeshLodCreator::GetLodPath(source, 1);
  const ezString sLod2 = ezMeshLodCreator::GetLodPath(source, 2);

  // a LOD of a mesh that is already at 50% has to be sparser than that mesh, not equal to it
  EZ_TEST_INT(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<ezInt64>(), 75);
  EZ_TEST_INT(ReadAssetProperty(sLod2, "MeshSimplification").ConvertTo<ezInt64>(), 88);
}

void ezEditorMeshLodTest::SubMeshVariantsDoNotShare()
{
  // One model file that several mesh assets import a different part of, which is how a set of plant
  // or rock variants is usually authored.
  const ezString sSource = MakePrivateSourceMesh("LodVariants");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  ezVariantDictionary extrasA;
  extrasA.Insert("MeshIncludeTags", "Variant_A");

  ezVariantDictionary extrasB;
  extrasB.Insert("MeshIncludeTags", "Variant_B");

  const ezUuid guidA = CreateMeshAsset("MeshLod/VariantA.ezMeshAsset", sSource, &extrasA);
  const ezUuid guidB = CreateMeshAsset("MeshLod/VariantB.ezMeshAsset", sSource, &extrasB);

  if (!EZ_TEST_BOOL(guidA.IsValid() && guidB.IsValid()))
    return;

  ezMeshLodSource sourceA, sourceB;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(guidA, sourceA).Succeeded()))
    return;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(guidB, sourceB).Succeeded()))
    return;

  EZ_TEST_STRING(sourceA.m_sMeshIncludeTags, "Variant_A");
  EZ_TEST_STRING(sourceB.m_sMeshIncludeTags, "Variant_B");

  // Each variant is named after its own mesh asset, not after the model file they share. Sharing it
  // would make one variant render the other's geometry at distance.
  EZ_TEST_BOOL_MSG(sourceA.m_sLodFolder.EndsWith("VariantA_data"), "A sub-mesh variant gets its own LOD folder.");
  EZ_TEST_BOOL_MSG(sourceB.m_sLodFolder.EndsWith("VariantB_data"), "A sub-mesh variant gets its own LOD folder.");
  EZ_TEST_BOOL(sourceA.m_sLodFolder != sourceB.m_sLodFolder);

  ezMeshLodOptions options;
  options.m_uiLodCount = 1;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(sourceA, options, uiCreated, uiSkipped).Succeeded()))
    return;

  EZ_TEST_INT(uiCreated, 1);

  // The second variant must still create its own, rather than finding the first one's and skipping.
  uiCreated = 0;
  uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(sourceB, options, uiCreated, uiSkipped).Succeeded()))
    return;

  EZ_TEST_INT_MSG(uiCreated, 1, "The variants must not claim each other's LOD assets.");
  EZ_TEST_INT(uiSkipped, 0);

  // and each LOD simplifies the part its own mesh asset uses
  EZ_TEST_STRING(ReadAssetProperty(ezMeshLodCreator::GetLodPath(sourceA, 1), "MeshIncludeTags").ConvertTo<ezString>(), "Variant_A");
  EZ_TEST_STRING(ReadAssetProperty(ezMeshLodCreator::GetLodPath(sourceB, 1), "MeshIncludeTags").ConvertTo<ezString>(), "Variant_B");
}

void ezEditorMeshLodTest::TransferredSettings()
{
  const ezString sSource = MakePrivateSourceMesh("LodSettings");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // every value differs from the mesh asset's default, so a transferred value cannot be mistaken
  // for a coincidental match
  const ezVec3 vOffset(1.0f, -2.0f, 3.5f);

  ezVariantDictionary extras;
  extras.Insert("MeshIncludeTags", "Render");
  extras.Insert("MeshExcludeTags", "NoRender;UCX_");
  extras.Insert("ImportTransform", (ezInt64)ezMeshImportTransform::Custom);
  extras.Insert("RightDir", (ezInt64)ezBasisAxis::PositiveZ);
  extras.Insert("UpDir", (ezInt64)ezBasisAxis::NegativeY);
  extras.Insert("FlipForwardDir", true);
  extras.Insert("PositionOffset", vOffset);
  extras.Insert("UniformScaling", 5.0f);

  const ezUuid meshGuid = CreateMeshAsset("MeshLod/Settings.ezMeshAsset", sSource, &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshLodSource source;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  ezMeshLodOptions options;
  options.m_uiLodCount = 1;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  const ezString sLod1 = ezMeshLodCreator::GetLodPath(source, 1);

  // A LOD has to sit in the same place, at the same size, built from the same part of the model as
  // the mesh it stands in for - otherwise the object visibly jumps when the LOD switches.
  EZ_TEST_STRING(ReadAssetProperty(sLod1, "MeshFile").ConvertTo<ezString>(), sSource);
  EZ_TEST_STRING(ReadAssetProperty(sLod1, "MeshIncludeTags").ConvertTo<ezString>(), "Render");
  EZ_TEST_STRING(ReadAssetProperty(sLod1, "MeshExcludeTags").ConvertTo<ezString>(), "NoRender;UCX_");
  EZ_TEST_INT(ReadAssetProperty(sLod1, "ImportTransform").ConvertTo<ezInt64>(), (ezInt64)ezMeshImportTransform::Custom);
  EZ_TEST_INT(ReadAssetProperty(sLod1, "RightDir").ConvertTo<ezInt64>(), (ezInt64)ezBasisAxis::PositiveZ);
  EZ_TEST_INT(ReadAssetProperty(sLod1, "UpDir").ConvertTo<ezInt64>(), (ezInt64)ezBasisAxis::NegativeY);
  EZ_TEST_BOOL(ReadAssetProperty(sLod1, "FlipForwardDir").ConvertTo<bool>() == true);
  EZ_TEST_VEC3(ReadAssetProperty(sLod1, "PositionOffset").ConvertTo<ezVec3>(), vOffset, 0.0001f);
  EZ_TEST_FLOAT(ReadAssetProperty(sLod1, "UniformScaling").ConvertTo<float>(), 5.0f, 0.0001f);
}

void ezEditorMeshLodTest::ExistingLods()
{
  const ezString sSource = MakePrivateSourceMesh("LodExisting");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshLod/Existing.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezString sLod1;

  {
    ezMeshLodSource source;
    ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();
    EZ_TEST_BOOL_MSG(!source.HasLod(1), "Nothing has been created yet.");

    sLod1 = ezMeshLodCreator::GetLodPath(source, 1);

    ezMeshLodOptions options;
    options.m_uiLodCount = 1;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 1);
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // a LOD may have been tuned by hand, so a second run must not throw that away
  {
    ezDocument* pLod = m_pApplication->m_pEditorApp->OpenDocument(sLod1, ezDocumentFlags::None);
    if (EZ_TEST_BOOL(pLod != nullptr))
    {
      ezDocumentObject* pProps = pLod->GetObjectManager()->GetRootObject()->GetChildren()[0];
      ezObjectAccessorBase* pAcc = pLod->GetObjectAccessor();

      pAcc->StartTransaction("Tune");
      pAcc->SetValueByName(pProps, "MeshSimplification", (ezInt64)62).AssertSuccess();
      pAcc->FinishTransaction();

      pLod->SaveDocument(true).AssertSuccess();
      pLod->GetDocumentManager()->CloseDocument(pLod);
    }
  }

  {
    ezMeshLodSource source;
    ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();
    EZ_TEST_BOOL_MSG(source.HasLod(1), "Gathering has to report the LOD that now exists.");

    ezMeshLodOptions options;
    options.m_uiLodCount = 1;
    options.m_bOverwriteExisting = false;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 1);

    EZ_TEST_INT_MSG(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<ezInt64>(), 62, "The hand-tuned value had to survive.");
  }

  // asking for it explicitly does replace it
  {
    ezMeshLodSource source;
    ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();

    ezMeshLodOptions options;
    options.m_uiLodCount = 1;
    options.m_bOverwriteExisting = true;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 1);
    EZ_TEST_INT(uiSkipped, 0);

    EZ_TEST_INT_MSG(ReadAssetProperty(sLod1, "MeshSimplification").ConvertTo<ezInt64>(), 50, "Replacing has to restore the generated value.");
  }

  // Raising the count adds only what is missing, rather than refusing because something is there.
  {
    ezMeshLodSource source;
    ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).AssertSuccess();

    ezMeshLodOptions options;
    options.m_uiLodCount = 3;
    options.m_bOverwriteExisting = false;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT_MSG(uiCreated, 2, "LOD-2 and LOD-3 were missing.");
    EZ_TEST_INT_MSG(uiSkipped, 1, "LOD-1 was already there.");
  }
}

void ezEditorMeshLodTest::PrefabPicksThemUp()
{
  const ezString sSource = MakePrivateSourceMesh("LodForPrefab");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshLodPrefab/ForPrefab.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  // Before the LODs exist, the mesh is just a mesh.
  {
    ezMeshPrefabSource prefabSource;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, prefabSource).Succeeded()))
      return;

    EZ_TEST_BOOL(prefabSource.m_LodGuids.IsEmpty());
    EZ_TEST_BOOL(prefabSource.GetDefaultRenderComponentType() == "ezMeshComponent"_ezsv);
  }

  ezMeshLodSource source;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  ezMeshLodOptions options;
  options.m_uiLodCount = 2;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped).Succeeded()))
    return;

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // creating a prefab afterwards has to find the LODs by itself and build a LOD component instead
  // of a plain mesh component, which is why the names and the folder are not a free choice
  {
    ezMeshPrefabSource prefabSource;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, prefabSource).Succeeded()))
      return;

    EZ_TEST_INT_MSG(prefabSource.m_LodGuids.GetCount(), 2, "The prefab tool has to find the LODs that were just created.");
    EZ_TEST_BOOL(prefabSource.GetDefaultRenderComponentType() == "ezLodMeshComponent"_ezsv);
  }
}

void ezEditorMeshLodTest::PrimitiveMesh()
{
  // A procedural primitive has no model file, so there is no geometry to simplify.
  // ezMeshPrimitive lives in EditorPluginAssets, which this test does not link. 6 is Sphere.
  ezVariantDictionary extras;
  extras.Insert("PrimitiveType", (ezInt64)6);

  const ezUuid meshGuid = CreateMeshAsset("MeshLod/Primitive.ezMeshAsset", "", &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshLodSource source;
  if (!EZ_TEST_BOOL(ezMeshLodCreator::GatherMeshLodSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_BOOL_MSG(source.m_bIsPrimitive, "A non-File primitive type has to be recognized.");
  EZ_TEST_BOOL(source.m_sMeshFile.IsEmpty());

  ezMeshLodOptions options;
  options.m_uiLodCount = 2;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  const ezStatus res = ezMeshLodCreator::CreateMeshLods(source, options, uiCreated, uiSkipped);

  EZ_TEST_BOOL_MSG(res.Failed(), "A primitive mesh has no source file, so this must fail.");
  EZ_TEST_INT(uiCreated, 0);
  EZ_TEST_BOOL_MSG(!ezOSFile::ExistsFile(ezMeshLodCreator::GetLodPath(source, 1)), "A failed run must not leave a document behind.");
}

void ezEditorMeshLodTest::MultipleMeshes()
{
  ezHybridArray<ezUuid, 4> meshes;
  ezHybridArray<ezMeshLodSource, 4> sources;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    ezStringBuilder sName;
    sName.SetFormat("LodBatch{}", i);

    const ezString sSource = MakePrivateSourceMesh(sName);
    if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
      return;

    ezStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshLodBatch/Batch{}.ezMeshAsset", i);

    const ezUuid guid = CreateMeshAsset(sAssetPath, sSource);
    if (!EZ_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    ezMeshLodSource& source = sources.ExpandAndGetRef();
    ezMeshLodCreator::GatherMeshLodSource(guid, source).AssertSuccess();
  }

  ezMeshLodOptions options;
  options.m_uiLodCount = 2;

  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLodsForAll(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT_MSG(uiCreated, 6, "Three meshes, two LODs each.");
    EZ_TEST_INT(uiSkipped, 0);

    for (const ezMeshLodSource& source : sources)
    {
      EZ_TEST_BOOL(ezOSFile::ExistsFile(ezMeshLodCreator::GetLodPath(source, 1)));
      EZ_TEST_BOOL(ezOSFile::ExistsFile(ezMeshLodCreator::GetLodPath(source, 2)));
    }
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // Running it again leaves everything alone rather than replacing what is there.
  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLodsForAll(meshes, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 6);
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    ezHybridArray<ezUuid, 2> mixed;
    mixed.PushBack(ezUuid::MakeUuid());

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLodsForAll(mixed, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 1);
  }

  // A LOD asset must never get LODs of its own, or the _data folders nest without end.
  {
    auto pLod = ezAssetCurator::GetSingleton()->FindSubAsset(ezMeshLodCreator::GetLodPath(sources[0], 1));
    if (EZ_TEST_BOOL(pLod.isValid()))
    {
      ezHybridArray<ezUuid, 2> lodOnly;
      lodOnly.PushBack(pLod->m_Data.m_Guid);

      ezUInt32 uiCreated = 0;
      ezUInt32 uiSkipped = 0;
      EZ_TEST_BOOL(ezMeshLodCreator::CreateMeshLodsForAll(lodOnly, options, uiCreated, uiSkipped).Succeeded());
      EZ_TEST_INT_MSG(uiCreated, 0, "A LOD must not get LODs of its own.");
      EZ_TEST_INT(uiSkipped, 1);
    }
  }
}
