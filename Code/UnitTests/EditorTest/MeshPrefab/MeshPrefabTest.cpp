#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <EditorTest/MeshPrefab/MeshPrefabTest.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorMeshPrefabTest s_EditorMeshPrefabTest;

namespace
{
  void GetComponentTypes(const ezDocumentObject* pObject, ezDynamicArray<ezString>& out_types)
  {
    out_types.Clear();

    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Components"_ezsv)
      {
        out_types.PushBack(pChild->GetType()->GetTypeName());
      }
    }
  }

  const ezDocumentObject* FindComponent(const ezDocumentObject* pObject, ezStringView sType)
  {
    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Components"_ezsv && pChild->GetType()->GetTypeName() == sType)
        return pChild;
    }

    return nullptr;
  }

  ezDynamicArray<const ezDocumentObject*> GetChildObjects(const ezDocumentObject* pObject)
  {
    ezDynamicArray<const ezDocumentObject*> res;

    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      if (pChild->GetParentProperty() == "Children"_ezsv)
        res.PushBack(pChild);
    }

    return res;
  }
} // namespace

const char* ezEditorMeshPrefabTest::GetTestName() const
{
  return "Mesh Prefab Tests";
}

void ezEditorMeshPrefabTest::SetupSubTests()
{
  AddSubTest("Simple Mesh", SubTests::ST_SimpleMesh);
  AddSubTest("LOD Mesh", SubTests::ST_LodMesh);
  AddSubTest("Box Collider", SubTests::ST_BoxCollider);
  AddSubTest("Collision Mesh", SubTests::ST_CollisionMesh);
  AddSubTest("Convex And Defaults", SubTests::ST_ConvexAndDefaults);
  AddSubTest("Existing Prefab", SubTests::ST_ExistingPrefab);
  AddSubTest("Keeps Open Documents", SubTests::ST_KeepsOpenDocuments);
  AddSubTest("Data Dir Relative Path", SubTests::ST_DataDirRelativePath);
  AddSubTest("Multiple Prefabs", SubTests::ST_MultiplePrefabs);
  AddSubTest("Animated LOD Component", SubTests::ST_AnimatedLodComponent);
}

ezResult ezEditorMeshPrefabTest::InitializeTest()
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

ezResult ezEditorMeshPrefabTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorMeshPrefabTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_SimpleMesh:
      SimpleMesh();
      break;
    case SubTests::ST_LodMesh:
      LodMesh();
      break;
    case SubTests::ST_BoxCollider:
      BoxCollider();
      break;
    case SubTests::ST_CollisionMesh:
      CollisionMesh();
      break;
    case SubTests::ST_ConvexAndDefaults:
      ConvexAndDefaults();
      break;
    case SubTests::ST_ExistingPrefab:
      ExistingPrefab();
      break;
    case SubTests::ST_KeepsOpenDocuments:
      KeepsOpenDocuments();
      break;
    case SubTests::ST_DataDirRelativePath:
      PrefabDataDirRelativePath();
      break;
    case SubTests::ST_MultiplePrefabs:
      MultiplePrefabs();
      break;
    case SubTests::ST_AnimatedLodComponent:
      AnimatedLodComponent();
      break;
  }

  return ezTestAppRun::Quit;
}

ezString ezEditorMeshPrefabTest::MakePrivateSourceMesh(const char* szName)
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

ezUuid ezEditorMeshPrefabTest::CreateMeshAsset(const char* szRelativePath, ezUInt8 uiSimplification, const char* szSourceFile)
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

    if (uiSimplification > 0)
    {
      pAcc->SetValueByName(pProps, "SimplifyMesh", true).AssertSuccess();
      pAcc->SetValueByName(pProps, "MeshSimplification", uiSimplification).AssertSuccess();
    }

    pAcc->FinishTransaction();
  }

  pDoc->SaveDocument(true).AssertSuccess();

  const ezUuid guid = pDoc->GetGuid();
  pDoc->GetDocumentManager()->CloseDocument(pDoc);

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // Bounds are recorded by a transform, under the asset hash current at that moment. The curator
  // keeps rehashing a newly created asset while indexing it, so a single transform tends to write
  // them under a hash that is stale by the time they are read. Retry until they can be read back.
  // Only this asset is transformed, transforming the generated prefabs would log unrelated errors.
  for (ezUInt32 i = 0; i < 10; ++i)
  {
    ezTransformStatus transformRes = ezAssetCurator::GetSingleton()->TransformAsset(guid, ezTransformFlags::TriggeredManually);
    EZ_IGNORE_UNUSED(transformRes);
    ProcessEvents(5);
    ezAssetCurator::GetSingleton()->MainThreadTick(true);

    auto pCheck = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (pCheck.isValid() && pCheck->m_pAssetInfo != nullptr && pCheck->m_pAssetInfo->GetTransformInfo() != nullptr)
      break;
  }

  return guid;
}

void ezEditorMeshPrefabTest::SimpleMesh()
{
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefab/Simple.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_BOOL(!source.m_bAnimated);
  EZ_TEST_BOOL(source.m_LodGuids.IsEmpty());
  EZ_TEST_BOOL_MSG(source.m_bHasBounds, "The mesh was transformed, so its bounds should be known.");
  EZ_TEST_BOOL(source.GetDefaultRenderComponentType() == "ezMeshComponent"_ezsv);

  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Simple.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = source.GetDefaultRenderComponentType();
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  EZ_TEST_BOOL(ezOSFile::ExistsFile(sPrefabPath));

  ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const ezDocumentObject* pRoot = pPrefab->GetObjectManager()->GetRootObject();
  auto topLevel = GetChildObjects(pRoot);
  if (!EZ_TEST_INT(topLevel.GetCount(), 1))
    return;

  const ezDocumentObject* pPrefabRoot = topLevel[0];
  EZ_TEST_STRING(pPrefabRoot->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>(), "<Prefab-Root>");

  const ezDocumentObject* pMeshComp = FindComponent(pPrefabRoot, "ezMeshComponent");
  if (!EZ_TEST_BOOL(pMeshComp != nullptr))
    return;

  ezStringBuilder sExpectedRef;
  sExpectedRef.SetFormat("{}", meshGuid);
  EZ_TEST_STRING(pMeshComp->GetTypeAccessor().GetValue("Mesh").ConvertTo<ezString>(), sExpectedRef);
}

void ezEditorMeshPrefabTest::LodMesh()
{
  // the layout the mesh import produces: the main asset plus LOD-N assets in a sibling _data folder
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefab/Lod.ezMeshAsset");
  const ezUuid lod1Guid = CreateMeshAsset("MeshPrefab/Lod_data/LOD-1.ezMeshAsset", 50);
  const ezUuid lod2Guid = CreateMeshAsset("MeshPrefab/Lod_data/LOD-2.ezMeshAsset", 75);

  if (!EZ_TEST_BOOL(meshGuid.IsValid() && lod1Guid.IsValid() && lod2Guid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!EZ_TEST_INT(source.m_LodGuids.GetCount(), 2))
    return;

  EZ_TEST_BOOL(source.m_LodGuids[0] == lod1Guid);
  EZ_TEST_BOOL(source.m_LodGuids[1] == lod2Guid);
  EZ_TEST_BOOL(source.GetDefaultRenderComponentType() == "ezLodMeshComponent"_ezsv);

  // A mesh asset renamed after import keeps its original _data folder, named after the source model
  // file. The LODs still have to be found.
  {
    ezStringBuilder sRenamedDir = m_sProjectPath;
    sRenamedDir.AppendPath("MeshPrefabRenamed");
    ezOSFile::CreateDirectoryStructure(sRenamedDir).AssertSuccess();

    const ezUuid renamedGuid = CreateMeshAsset("MeshPrefabRenamed/Renamed.ezMeshAsset");
    const ezUuid renamedLod1 = CreateMeshAsset("MeshPrefabRenamed/Cube_data/LOD-1.ezMeshAsset", 50);

    if (EZ_TEST_BOOL(renamedGuid.IsValid() && renamedLod1.IsValid()))
    {
      ezMeshPrefabSource renamedSource;
      if (EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(renamedGuid, renamedSource).Succeeded()))
      {
        if (EZ_TEST_INT(renamedSource.m_LodGuids.GetCount(), 1))
        {
          EZ_TEST_BOOL(renamedSource.m_LodGuids[0] == renamedLod1);
        }
      }
    }
  }

  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Lod.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = source.GetDefaultRenderComponentType();
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const ezDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
  const ezDocumentObject* pLodComp = FindComponent(pPrefabRoot, "ezLodMeshComponent");
  if (!EZ_TEST_BOOL(pLodComp != nullptr))
    return;

  // LOD 0 is the mesh asset itself, so there is one entry more than there are LOD assets
  ezHybridArray<const ezDocumentObject*, 4> lods;
  for (const ezDocumentObject* pChild : pLodComp->GetChildren())
  {
    if (pChild->GetParentProperty() == "Meshes"_ezsv)
      lods.PushBack(pChild);
  }

  if (!EZ_TEST_INT(lods.GetCount(), 3))
    return;

  const ezUuid expected[] = {meshGuid, lod1Guid, lod2Guid};
  float fPrevThreshold = 2.0f;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    ezStringBuilder sExpectedRef;
    sExpectedRef.SetFormat("{}", expected[i]);
    EZ_TEST_STRING(lods[i]->GetTypeAccessor().GetValue("Mesh").ConvertTo<ezString>(), sExpectedRef);

    // thresholds have to decrease, otherwise the component never switches LOD
    const float fThreshold = lods[i]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>();
    EZ_TEST_BOOL(fThreshold < fPrevThreshold);
    fPrevThreshold = fThreshold;
  }

  EZ_TEST_FLOAT(lods[0]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.2f, 0.001f);
  EZ_TEST_FLOAT(lods[1]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.1f, 0.001f);

  // the last LOD has to reach all the way out, or the mesh disappears in the distance
  EZ_TEST_FLOAT(lods[2]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.0f, 0.001f);

  const float fRadius = pLodComp->GetTypeAccessor().GetValue("BoundsRadius").ConvertTo<float>();
  EZ_TEST_FLOAT(fRadius, source.m_fBoundsRadius, 0.001f);
  EZ_TEST_BOOL_MSG(fRadius != 1.0f, "The bounds radius should come from the mesh, not stay at the default.");
}

void ezEditorMeshPrefabTest::BoxCollider()
{
  if (!ezMeshPrefabCreator::IsPhysicsAvailable())
  {
    ezLog::Info("Jolt is not available, skipping the collider test.");
    return;
  }

  const ezUuid meshGuid = CreateMeshAsset("MeshPrefab/Box.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!EZ_TEST_BOOL_MSG(source.m_bHasBounds, "A box collider cannot be sized without bounds."))
    return;

  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/Box.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "ezMeshComponent";
  options.m_Physics = ezMeshPrefabPhysics::StaticBox;
  options.m_uiCollisionLayer = 3;
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const ezDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];

  const ezDocumentObject* pActor = FindComponent(pPrefabRoot, "ezJoltStaticActorComponent");
  if (!EZ_TEST_BOOL(pActor != nullptr))
    return;

  EZ_TEST_INT(pActor->GetTypeAccessor().GetValue("CollisionLayer").ConvertTo<ezUInt32>(), 3);

  auto children = GetChildObjects(pPrefabRoot);
  if (!EZ_TEST_INT(children.GetCount(), 1))
    return;

  const ezDocumentObject* pShape = FindComponent(children[0], "ezJoltShapeBoxComponent");
  if (!EZ_TEST_BOOL(pShape != nullptr))
    return;

  const ezVec3 vHalfExtents = pShape->GetTypeAccessor().GetValue("HalfExtents").ConvertTo<ezVec3>();
  EZ_TEST_VEC3(vHalfExtents, source.m_vBoundsHalfExtents, 0.001f);

  const ezVec3 vPos = children[0]->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<ezVec3>();
  EZ_TEST_VEC3(vPos, source.m_vBoundsCenter, 0.001f);
}

void ezEditorMeshPrefabTest::CollisionMesh()
{
  if (!ezMeshPrefabCreator::IsPhysicsAvailable())
  {
    ezLog::Info("Jolt is not available, skipping the collision mesh test.");
    return;
  }

  const ezUuid meshGuid = CreateMeshAsset("MeshPrefab/ColMesh.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_STRING(source.m_sMeshFile, "Meshes/Cube.obj");

  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefab/ColMesh.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "ezMeshComponent";
  options.m_Physics = ezMeshPrefabPhysics::StaticTriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  ezStringBuilder sColMeshPath = m_sProjectPath;
  sColMeshPath.AppendPath("MeshPrefab/ColMesh.ezJoltCollisionMeshAsset");
  EZ_TEST_BOOL(ezOSFile::ExistsFile(sColMeshPath));

  ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  const ezDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
  const ezDocumentObject* pActor = FindComponent(pPrefabRoot, "ezJoltStaticActorComponent");
  if (!EZ_TEST_BOOL(pActor != nullptr))
    return;

  // a triangle mesh collider needs no separate shape component, the actor references the mesh itself
  const ezString sColMeshRef = pActor->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<ezString>();
  EZ_TEST_BOOL_MSG(!sColMeshRef.IsEmpty(), "The actor should reference the generated collision mesh.");
  EZ_TEST_INT(GetChildObjects(pPrefabRoot).GetCount(), 0);

  // running it again has to reuse that asset rather than create a second one
  ezStringBuilder sPrefabPath2 = m_sProjectPath;
  sPrefabPath2.AppendPath("MeshPrefab/ColMesh2.ezPrefab");

  ezMeshPrefabOptions options2 = options;
  options2.m_sPrefabPath = sPrefabPath2;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options2).Succeeded()))
    return;

  ezDocument* pPrefab2 = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath2, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab2 != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab2->GetDocumentManager()->CloseDocument(pPrefab2));

  const ezDocumentObject* pPrefabRoot2 = GetChildObjects(pPrefab2->GetObjectManager()->GetRootObject())[0];
  const ezDocumentObject* pActor2 = FindComponent(pPrefabRoot2, "ezJoltStaticActorComponent");
  if (!EZ_TEST_BOOL(pActor2 != nullptr))
    return;

  EZ_TEST_STRING(pActor2->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<ezString>(), sColMeshRef);
}

void ezEditorMeshPrefabTest::ConvexAndDefaults()
{
  if (!ezMeshPrefabCreator::IsPhysicsAvailable())
  {
    ezLog::Info("Jolt is not available, skipping the convex hull test.");
    return;
  }

  // Collision meshes are matched by source file, and every other mesh here is built from Cube.obj.
  // Without a private source this would find the colliders the other sub-tests generated.
  const ezString sSource = MakePrivateSourceMesh("ConvexCube");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshPrefabConvex/Convex.ezMeshAsset", 0, sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  {
    ezMeshPrefabSource source;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
      return;

    EZ_TEST_BOOL(source.GetDefaultPhysics() == ezMeshPrefabPhysics::None);
    EZ_TEST_BOOL(!source.m_ExistingConvexColMesh.IsValid());

    ezStringBuilder sPrefabPath = m_sProjectPath;
    sPrefabPath.AppendPath("MeshPrefabConvex/Convex.ezPrefab");

    ezMeshPrefabOptions options;
    options.m_sPrefabPath = sPrefabPath;
    options.m_sRenderComponentType = "ezMeshComponent";
    options.m_Physics = ezMeshPrefabPhysics::StaticConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
      return;

    // a convex hull is a static actor plus a shape component, unlike a triangle mesh
    ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
    if (!EZ_TEST_BOOL(pPrefab != nullptr))
      return;

    EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

    const ezDocumentObject* pPrefabRoot = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject())[0];
    EZ_TEST_BOOL(FindComponent(pPrefabRoot, "ezJoltStaticActorComponent") != nullptr);

    const ezDocumentObject* pShape = FindComponent(pPrefabRoot, "ezJoltShapeConvexHullComponent");
    if (!EZ_TEST_BOOL(pShape != nullptr))
      return;

    EZ_TEST_BOOL(!pShape->GetTypeAccessor().GetValue("CollisionMesh").ConvertTo<ezString>().IsEmpty());
  }

  // the convex collision mesh now exists, so it should drive the default on a second run
  {
    ezMeshPrefabSource source;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
      return;

    EZ_TEST_BOOL(source.m_ExistingConvexColMesh.IsValid());
    EZ_TEST_BOOL(source.GetDefaultPhysics() == ezMeshPrefabPhysics::StaticConvexHull);
  }
}

void ezEditorMeshPrefabTest::ExistingPrefab()
{
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefabExisting/Existing.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefabExisting/Existing.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "ezMeshComponent";
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  const ezStatus second = ezMeshPrefabCreator::CreateMeshPrefab(source, options);
  EZ_TEST_BOOL_MSG(second.Failed(), "Creating a prefab over an existing one should fail, not overwrite it.");
  EZ_TEST_BOOL(!second.GetMessageString().IsEmpty());
}

void ezEditorMeshPrefabTest::KeepsOpenDocuments()
{
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefabOpen/Open.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezStringBuilder sMeshPath = m_sProjectPath;
  sMeshPath.AppendPath("MeshPrefabOpen/Open.ezMeshAsset");

  // Opened without a window, which is what the curator does while transforming. Reading a property
  // out of it must not close it.
  ezDocument* pOpened = m_pApplication->m_pEditorApp->OpenDocument(sMeshPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pOpened != nullptr))
    return;

  ezMeshPrefabSource source;
  EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded());
  EZ_TEST_STRING(source.m_sMeshFile, "Meshes/Cube.obj");

  const ezDocument* pStillOpen = ezDocumentManager::GetDocumentByGuid(meshGuid);
  EZ_TEST_BOOL_MSG(pStillOpen == pOpened, "Gathering must not close a document that was already open.");

  if (pStillOpen == pOpened)
  {
    pOpened->GetDocumentManager()->CloseDocument(pOpened);
  }
}

void ezEditorMeshPrefabTest::PrefabDataDirRelativePath()
{
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefab/RelPath.ezMeshAsset");
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  const ezString sAbsolute = ezMeshPrefabCreator::SuggestPrefabPath(source);
  const ezString sDisplay = ezMeshPrefabCreator::MakeDisplayPath(sAbsolute);

  // What the dialog shows has to be the short form, not the full path off the drive root.
  EZ_TEST_BOOL_MSG(!ezPathUtils::IsAbsolutePath(sDisplay), "The display path must be relative.");
  EZ_TEST_BOOL_MSG(sDisplay.FindSubString("MeshPrefab/RelPath") != nullptr, "The display path must still name the file.");

  // and it has to round trip, or the dialog cannot hand it back to the creator
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshPrefabCreator::ResolveDisplayPath(sDisplay, sResolved).Succeeded());
    EZ_TEST_STRING(sResolved, sAbsolute);
  }

  // an absolute path stays valid input, which is what the file browse button produces
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshPrefabCreator::ResolveDisplayPath(sAbsolute, sResolved).Succeeded());
    EZ_TEST_STRING(sResolved, sAbsolute);
  }

  // a path that names no data directory has to be refused rather than written somewhere unexpected
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshPrefabCreator::ResolveDisplayPath("NoSuchDataDir/Thing.ezPrefab", sResolved).Failed());
    EZ_TEST_BOOL(ezMeshPrefabCreator::ResolveDisplayPath("", sResolved).Failed());
  }

  // creating from the relative form has to put the file exactly where the absolute form would
  {
    ezMeshPrefabOptions options;
    options.m_sPrefabPath = sDisplay;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
      return;

    EZ_TEST_BOOL(ezOSFile::ExistsFile(sAbsolute));
  }

  // an unresolvable path must fail instead of creating something
  {
    ezMeshPrefabOptions options;
    options.m_sPrefabPath = "NoSuchDataDir/Thing.ezPrefab";
    options.m_bOpenAfterCreate = false;

    EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Failed());
  }
}

void ezEditorMeshPrefabTest::MultiplePrefabs()
{
  ezHybridArray<ezUuid, 4> meshes;
  ezHybridArray<ezString, 4> expectedPrefabs;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    ezStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshPrefabBatch/Batch{}.ezMeshAsset", i);

    const ezUuid guid = CreateMeshAsset(sAssetPath);
    if (!EZ_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    ezStringBuilder sPrefab = m_sProjectPath;
    sPrefab.AppendPath(sAssetPath);
    sPrefab.ChangeFileExtension("ezPrefab");
    expectedPrefabs.PushBack(sPrefab);
  }

  ezMeshPrefabOptions options;
  options.m_bOpenAfterCreate = false;

  // no path: each prefab has to end up next to its own mesh
  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefabs(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT(uiCreated, 3);
    EZ_TEST_INT(uiSkipped, 0);

    for (const ezString& sPrefab : expectedPrefabs)
    {
      EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sPrefab), "Every mesh has to get a prefab at its own default path.");
    }
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // Running it again must not pile up numbered duplicates. A mesh that already has a prefab is done,
  // which is why such a mesh is skipped rather than failing the run.
  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefabs(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 3);

    for (const ezString& sPrefab : expectedPrefabs)
    {
      ezStringBuilder sSecondName(ezPathUtils::GetFileName(sPrefab), "2");

      ezStringBuilder sSecond = sPrefab;
      sSecond.ChangeFileName(sSecondName);

      EZ_TEST_BOOL_MSG(!ezOSFile::ExistsFile(sSecond), "A second run must not create a numbered duplicate.");
    }
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    ezHybridArray<ezUuid, 2> mixed;
    mixed.PushBack(ezUuid::MakeUuid());

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefabs(mixed, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 1);
  }
}

void ezEditorMeshPrefabTest::AnimatedLodComponent()
{
  // The LOD folder is found by the mesh asset name, so this mesh needs a name of its own.
  const ezUuid meshGuid = CreateMeshAsset("MeshPrefabAnimLod/AnimLod.ezMeshAsset");
  const ezUuid lod1Guid = CreateMeshAsset("MeshPrefabAnimLod/AnimLod_data/LOD-1.ezMeshAsset", 50);

  if (!EZ_TEST_BOOL(meshGuid.IsValid() && lod1Guid.IsValid()))
    return;

  ezMeshPrefabSource source;
  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::GatherMeshPrefabSource(meshGuid, source).Succeeded()))
    return;

  if (!EZ_TEST_INT(source.m_LodGuids.GetCount(), 1))
    return;

  // A static mesh with LODs still defaults to the static LOD component.
  EZ_TEST_BOOL(source.GetDefaultRenderComponentType() == "ezLodMeshComponent"_ezsv);

  // The animated LOD component is only offered for animated meshes. What matters here is that it is
  // built correctly when asked for, including the LOD element type, which differs from the static one.
  ezStringBuilder sPrefabPath = m_sProjectPath;
  sPrefabPath.AppendPath("MeshPrefabAnimLod/AnimLod.ezPrefab");

  ezMeshPrefabOptions options;
  options.m_sPrefabPath = sPrefabPath;
  options.m_sRenderComponentType = "ezLodAnimatedMeshComponent";
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshPrefabCreator::CreateMeshPrefab(source, options).Succeeded()))
    return;

  ezDocument* pPrefab = m_pApplication->m_pEditorApp->OpenDocument(sPrefabPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pPrefab != nullptr))
    return;

  EZ_SCOPE_EXIT(pPrefab->GetDocumentManager()->CloseDocument(pPrefab));

  auto topLevel = GetChildObjects(pPrefab->GetObjectManager()->GetRootObject());
  if (!EZ_TEST_INT(topLevel.GetCount(), 1))
    return;

  const ezDocumentObject* pComp = FindComponent(topLevel[0], "ezLodAnimatedMeshComponent");
  if (!EZ_TEST_BOOL(pComp != nullptr))
    return;

  // LOD 0 is the mesh itself, LOD 1 is the sibling that was found
  ezHybridArray<const ezDocumentObject*, 4> lods;
  for (const ezDocumentObject* pChild : pComp->GetChildren())
  {
    if (pChild->GetParentProperty() == "Meshes"_ezsv)
      lods.PushBack(pChild);
  }

  if (!EZ_TEST_INT(lods.GetCount(), 2))
    return;

  // The two LOD components have separate element types with identical property names. Using the
  // static one here would build a document the animated component cannot read.
  EZ_TEST_STRING(lods[0]->GetTypeAccessor().GetType()->GetTypeName(), "ezLodAnimatedMeshLod");

  ezStringBuilder sExpectedRef;
  sExpectedRef.SetFormat("{}", meshGuid);
  EZ_TEST_STRING(lods[0]->GetTypeAccessor().GetValue("Mesh").ConvertTo<ezString>(), sExpectedRef);

  sExpectedRef.SetFormat("{}", lod1Guid);
  EZ_TEST_STRING(lods[1]->GetTypeAccessor().GetValue("Mesh").ConvertTo<ezString>(), sExpectedRef);

  // the last LOD has to reach out to the horizon, or the object disappears at a distance
  EZ_TEST_FLOAT(lods[1]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>(), 0.0f, 0.0001f);
  EZ_TEST_BOOL(lods[0]->GetTypeAccessor().GetValue("Threshold").ConvertTo<float>() > 0.0f);
}
