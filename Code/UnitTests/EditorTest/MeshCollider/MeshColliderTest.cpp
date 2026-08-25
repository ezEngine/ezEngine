#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <EditorTest/MeshCollider/MeshColliderTest.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Declarations.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorMeshColliderTest s_EditorMeshColliderTest;

const char* ezEditorMeshColliderTest::GetTestName() const
{
  return "Mesh Collider Tests";
}

void ezEditorMeshColliderTest::SetupSubTests()
{
  AddSubTest("Triangle Mesh", SubTests::ST_TriangleMesh);
  AddSubTest("Convex Mesh", SubTests::ST_ConvexMesh);
  AddSubTest("Transferred Settings", SubTests::ST_TransferredSettings);
  AddSubTest("Existing Collider", SubTests::ST_ExistingCollider);
  AddSubTest("Primitive Mesh", SubTests::ST_PrimitiveMesh);
  AddSubTest("Keeps Open Documents", SubTests::ST_KeepsOpenDocuments);
  AddSubTest("Simplification Settings", SubTests::ST_SimplificationSettings);
  AddSubTest("Data Dir Relative Path", SubTests::ST_DataDirRelativePath);
  AddSubTest("Multiple Meshes", SubTests::ST_MultipleMeshes);
  AddSubTest("Shared Source File", SubTests::ST_SharedSourceFile);
  AddSubTest("Surface", SubTests::ST_Surface);
}

ezResult ezEditorMeshColliderTest::InitializeTest()
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

ezResult ezEditorMeshColliderTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorMeshColliderTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_TriangleMesh:
      TriangleMesh();
      break;
    case SubTests::ST_ConvexMesh:
      ConvexMesh();
      break;
    case SubTests::ST_TransferredSettings:
      TransferredSettings();
      break;
    case SubTests::ST_ExistingCollider:
      ExistingCollider();
      break;
    case SubTests::ST_PrimitiveMesh:
      PrimitiveMesh();
      break;
    case SubTests::ST_KeepsOpenDocuments:
      KeepsOpenDocuments();
      break;
    case SubTests::ST_SimplificationSettings:
      SimplificationSettings();
      break;
    case SubTests::ST_DataDirRelativePath:
      DataDirRelativePath();
      break;
    case SubTests::ST_MultipleMeshes:
      MultipleMeshes();
      break;
    case SubTests::ST_Surface:
      Surface();
      break;
    case SubTests::ST_SharedSourceFile:
      SharedSourceFile();
      break;
  }

  return ezTestAppRun::Quit;
}

ezString ezEditorMeshColliderTest::MakePrivateSourceMesh(const char* szName)
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

ezUuid ezEditorMeshColliderTest::CreateMeshAsset(const char* szRelativePath, const char* szSourceFile, const ezVariantDictionary* pExtraProperties)
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

ezVariant ezEditorMeshColliderTest::ReadColliderProperty(ezStringView sAbsPath, ezStringView sProperty)
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

void ezEditorMeshColliderTest::TriangleMesh()
{
  // Colliders are matched to a mesh by source file, so every sub-test needs its own copy of the
  // source, or it finds the colliders that the other sub-tests generated.
  const ezString sSource = MakePrivateSourceMesh("ColliderTriangle");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Triangle.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_BOOL(!source.m_bAnimated);
  EZ_TEST_BOOL(!source.m_bIsPrimitive);
  EZ_TEST_STRING(source.m_sMeshFile, sSource);
  EZ_TEST_BOOL(!source.GetExisting(ezMeshColliderKind::TriangleMesh).IsValid());

  const ezString sSuggested = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);
  EZ_TEST_STRING(ezPathUtils::GetFileExtension(sSuggested), "ezJoltCollisionMeshAsset");

  ezMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = ezMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!EZ_TEST_BOOL(ezOSFile::ExistsFile(sSuggested)))
    return;

  // the source file is what makes the collider describe the same geometry as the mesh
  EZ_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<ezString>(), sSource);

  // a triangle mesh document must not be flagged as convex, or it builds the wrong shape
  EZ_TEST_BOOL(ReadColliderProperty(sSuggested, "IsConvexMesh").ConvertTo<bool>() == false);
}

void ezEditorMeshColliderTest::ConvexMesh()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderConvex");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Convex.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const ezString sSuggested = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::ConvexHull);
  EZ_TEST_STRING(ezPathUtils::GetFileExtension(sSuggested), "ezJoltConvexCollisionMeshAsset");

  ezMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = ezMeshColliderKind::ConvexHull;
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!EZ_TEST_BOOL(ezOSFile::ExistsFile(sSuggested)))
    return;

  EZ_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<ezString>(), sSource);

  // the convex flag comes from the document type, not from the transferred properties
  EZ_TEST_BOOL(ReadColliderProperty(sSuggested, "IsConvexMesh").ConvertTo<bool>() == true);
}

void ezEditorMeshColliderTest::TransferredSettings()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderSettings");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // every value differs from the collision mesh asset's default, so a transferred value cannot be
  // mistaken for a coincidental match
  const ezVec3 vOffset(1.0f, -2.0f, 3.5f);

  ezVariantDictionary extras;
  extras.Insert("MeshIncludeTags", "Collide");
  extras.Insert("MeshExcludeTags", "NoCollide;UCX_");
  extras.Insert("ImportTransform", (ezInt64)ezMeshImportTransform::Custom);
  extras.Insert("RightDir", (ezInt64)ezBasisAxis::PositiveZ);
  extras.Insert("UpDir", (ezInt64)ezBasisAxis::NegativeY);
  extras.Insert("FlipForwardDir", true);
  extras.Insert("PositionOffset", vOffset);
  extras.Insert("UniformScaling", 5.0f);

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Settings.ezMeshAsset", sSource, &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const ezString sSuggested = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);

  ezMeshColliderOptions options;
  options.m_sColliderPath = sSuggested;
  options.m_Kind = ezMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
    return;

  if (!EZ_TEST_BOOL(ezOSFile::ExistsFile(sSuggested)))
    return;

  // the collider has to describe the same geometry, in the same place, at the same size, as the mesh
  EZ_TEST_STRING(ReadColliderProperty(sSuggested, "MeshFile").ConvertTo<ezString>(), sSource);
  EZ_TEST_STRING(ReadColliderProperty(sSuggested, "MeshIncludeTags").ConvertTo<ezString>(), "Collide");
  EZ_TEST_STRING(ReadColliderProperty(sSuggested, "MeshExcludeTags").ConvertTo<ezString>(), "NoCollide;UCX_");
  EZ_TEST_INT(ReadColliderProperty(sSuggested, "ImportTransform").ConvertTo<ezInt64>(), (ezInt64)ezMeshImportTransform::Custom);
  EZ_TEST_INT(ReadColliderProperty(sSuggested, "RightDir").ConvertTo<ezInt64>(), (ezInt64)ezBasisAxis::PositiveZ);
  EZ_TEST_INT(ReadColliderProperty(sSuggested, "UpDir").ConvertTo<ezInt64>(), (ezInt64)ezBasisAxis::NegativeY);
  EZ_TEST_BOOL(ReadColliderProperty(sSuggested, "FlipForwardDir").ConvertTo<bool>() == true);
  EZ_TEST_VEC3(ReadColliderProperty(sSuggested, "PositionOffset").ConvertTo<ezVec3>(), vOffset, 0.0001f);
  EZ_TEST_FLOAT(ReadColliderProperty(sSuggested, "UniformScaling").ConvertTo<float>(), 5.0f, 0.0001f);
}

void ezEditorMeshColliderTest::ExistingCollider()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderExisting");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Existing.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezString sColliderPath;

  {
    ezMeshColliderSource source;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
      return;

    EZ_TEST_BOOL(!source.GetExisting(ezMeshColliderKind::TriangleMesh).IsValid());

    sColliderPath = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sColliderPath;
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    const ezStatus second = ezMeshColliderCreator::CreateMeshCollider(source, options);
    EZ_TEST_BOOL_MSG(second.Failed(), "Creating a collider over an existing file should fail, not overwrite it.");
    EZ_TEST_BOOL(!second.GetMessageString().IsEmpty());
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // gathering again has to report the collider that now exists, so the dialog can point it out
  {
    ezMeshColliderSource source;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
      return;

    EZ_TEST_BOOL(source.GetExisting(ezMeshColliderKind::TriangleMesh).IsValid());
    EZ_TEST_BOOL_MSG(!source.GetExisting(ezMeshColliderKind::ConvexHull).IsValid(), "Only a triangle mesh was created.");
  }

  // and the suggested path must move on, rather than proposing a name that cannot be used
  {
    ezMeshColliderSource source;
    ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).AssertSuccess();

    const ezString sNext = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);
    EZ_TEST_BOOL(sNext != sColliderPath);
    EZ_TEST_BOOL(!ezOSFile::ExistsFile(sNext));
  }
}

void ezEditorMeshColliderTest::PrimitiveMesh()
{
  // A procedural primitive has no model file, so there is nothing to build a collision mesh from.
  // ezMeshPrimitive lives in EditorPluginAssets, which this test does not link. 6 is Sphere.
  ezVariantDictionary extras;
  extras.Insert("PrimitiveType", (ezInt64)6);

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Primitive.ezMeshAsset", "", &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  EZ_TEST_BOOL_MSG(source.m_bIsPrimitive, "A non-File primitive type has to be recognized.");
  EZ_TEST_BOOL(source.m_sMeshFile.IsEmpty());

  ezStringBuilder sColliderPath = m_sProjectPath;
  sColliderPath.AppendPath("MeshCollider/Primitive.ezJoltCollisionMeshAsset");

  ezMeshColliderOptions options;
  options.m_sColliderPath = sColliderPath;
  options.m_Kind = ezMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  const ezStatus res = ezMeshColliderCreator::CreateMeshCollider(source, options);
  EZ_TEST_BOOL_MSG(res.Failed(), "A primitive mesh has no source file, so this must fail.");
  EZ_TEST_BOOL_MSG(!ezOSFile::ExistsFile(sColliderPath), "A failed creation must not leave a document behind.");
}

void ezEditorMeshColliderTest::KeepsOpenDocuments()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderOpen");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshColliderOpen/Open.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezStringBuilder sMeshPath = m_sProjectPath;
  sMeshPath.AppendPath("MeshColliderOpen/Open.ezMeshAsset");

  // Opened without a window, which is what the curator does while transforming. Reading properties
  // out of it must not close it.
  ezDocument* pOpened = m_pApplication->m_pEditorApp->OpenDocument(sMeshPath, ezDocumentFlags::None);
  if (!EZ_TEST_BOOL(pOpened != nullptr))
    return;

  ezMeshColliderSource source;
  EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded());
  EZ_TEST_STRING(source.m_sMeshFile, sSource);

  const ezDocument* pStillOpen = ezDocumentManager::GetDocumentByGuid(meshGuid);
  EZ_TEST_BOOL_MSG(pStillOpen == pOpened, "Gathering must not close a document that was already open.");

  if (pStillOpen == pOpened)
  {
    pOpened->GetDocumentManager()->CloseDocument(pOpened);
  }
}

void ezEditorMeshColliderTest::SimplificationSettings()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderSimplify");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  // Again every value differs from the collision mesh asset's default. MaxSimplificationError
  // defaults to 5 on the mesh asset but to 20 on the collision mesh asset.
  ezVariantDictionary extras;
  extras.Insert("SimplifyMesh", true);
  extras.Insert("MeshSimplification", (ezInt64)33);
  extras.Insert("MaxSimplificationError", (ezInt64)7);
  extras.Insert("NormalWeight", 0.75f);
  extras.Insert("AggressiveSimplification", true);

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Simplify.ezMeshAsset", sSource, &extras);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  // A triangle mesh keeps the source geometry, so simplifying it changes the collision shape.
  {
    const ezString sPath = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    EZ_TEST_BOOL(ReadColliderProperty(sPath, "SimplifyMesh").ConvertTo<bool>() == true);
    EZ_TEST_INT(ReadColliderProperty(sPath, "MeshSimplification").ConvertTo<ezInt64>(), 33);
    EZ_TEST_INT(ReadColliderProperty(sPath, "MaxSimplificationError").ConvertTo<ezInt64>(), 7);
    EZ_TEST_FLOAT(ReadColliderProperty(sPath, "NormalWeight").ConvertTo<float>(), 0.75f, 0.0001f);
    EZ_TEST_BOOL(ReadColliderProperty(sPath, "AggressiveSimplification").ConvertTo<bool>() == true);
  }

  // A convex hull is built from the hull of the vertices, so simplifying the source first changes
  // nothing about it. The collision mesh asset hides these options for a convex mesh.
  {
    const ezString sPath = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::ConvexHull);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = ezMeshColliderKind::ConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    EZ_TEST_BOOL_MSG(ReadColliderProperty(sPath, "SimplifyMesh").ConvertTo<bool>() == false, "A convex hull must not take over the simplification settings.");
    EZ_TEST_INT(ReadColliderProperty(sPath, "MeshSimplification").ConvertTo<ezInt64>(), 50);

    // but the geometry settings still have to be there, this is not a general opt out
    EZ_TEST_STRING(ReadColliderProperty(sPath, "MeshFile").ConvertTo<ezString>(), sSource);
  }
}

void ezEditorMeshColliderTest::DataDirRelativePath()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderRelPath");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/RelPath.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  const ezString sAbsolute = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);
  const ezString sDisplay = ezMeshColliderCreator::MakeDisplayPath(sAbsolute);

  // What the dialog shows has to be the short form, not the full path off the drive root.
  EZ_TEST_BOOL_MSG(!ezPathUtils::IsAbsolutePath(sDisplay), "The display path must be relative.");
  EZ_TEST_BOOL_MSG(sDisplay.FindSubString("MeshCollider/RelPath") != nullptr, "The display path must still name the file.");

  // and it has to round trip, or the dialog cannot hand it back to the creator
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshColliderCreator::ResolveDisplayPath(sDisplay, sResolved).Succeeded());
    EZ_TEST_STRING(sResolved, sAbsolute);
  }

  // an absolute path stays valid input, which is what the file browse button produces
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshColliderCreator::ResolveDisplayPath(sAbsolute, sResolved).Succeeded());
    EZ_TEST_STRING(sResolved, sAbsolute);
  }

  // a path that names no data directory has to be refused rather than written somewhere unexpected
  {
    ezStringBuilder sResolved;
    EZ_TEST_BOOL(ezMeshColliderCreator::ResolveDisplayPath("NoSuchDataDir/Thing.ezJoltCollisionMeshAsset", sResolved).Failed());
    EZ_TEST_BOOL(ezMeshColliderCreator::ResolveDisplayPath("", sResolved).Failed());
  }

  // and creating from the relative form has to put the file exactly where the absolute form would
  {
    ezMeshColliderOptions options;
    options.m_sColliderPath = sDisplay;
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    EZ_TEST_BOOL(ezOSFile::ExistsFile(sAbsolute));
  }

  // an unresolvable path must fail instead of creating something
  {
    ezMeshColliderOptions options;
    options.m_sColliderPath = "NoSuchDataDir/Thing.ezJoltCollisionMeshAsset";
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Failed());
  }
}

void ezEditorMeshColliderTest::MultipleMeshes()
{
  ezHybridArray<ezUuid, 4> meshes;
  ezHybridArray<ezString, 4> expectedColliders;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    ezStringBuilder sName;
    sName.SetFormat("ColliderBatch{}", i);

    const ezString sSource = MakePrivateSourceMesh(sName);
    if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
      return;

    ezStringBuilder sAssetPath;
    sAssetPath.SetFormat("MeshCollider/Batch{}.ezMeshAsset", i);

    const ezUuid guid = CreateMeshAsset(sAssetPath, sSource);
    if (!EZ_TEST_BOOL(guid.IsValid()))
      return;

    meshes.PushBack(guid);

    ezStringBuilder sCollider = m_sProjectPath;
    sCollider.AppendPath(sAssetPath);
    sCollider.ChangeFileExtension("ezJoltCollisionMeshAsset");
    expectedColliders.PushBack(sCollider);
  }

  ezMeshColliderOptions options;
  options.m_Kind = ezMeshColliderKind::TriangleMesh;
  options.m_bOpenAfterCreate = false;

  // no path: each collider has to end up next to its own mesh
  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshColliders(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT(uiCreated, 3);
    EZ_TEST_INT(uiSkipped, 0);

    for (const ezString& sCollider : expectedColliders)
    {
      EZ_TEST_BOOL_MSG(ezOSFile::ExistsFile(sCollider), "Every mesh has to get a collider at its own default path.");
    }
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  // running it again must not pile up numbered duplicates, a mesh that already has a collider is done
  {
    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshColliders(meshes, options, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 3);

    for (const ezString& sCollider : expectedColliders)
    {
      ezStringBuilder sSecondName(ezPathUtils::GetFileName(sCollider), "2");

      ezStringBuilder sSecond = sCollider;
      sSecond.ChangeFileName(sSecondName);

      EZ_TEST_BOOL_MSG(!ezOSFile::ExistsFile(sSecond), "A second run must not create a numbered duplicate.");
    }
  }

  // A convex collider is a different kind, so none of them exists yet and all three are created.
  {
    ezMeshColliderOptions convex = options;
    convex.m_Kind = ezMeshColliderKind::ConvexHull;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshColliders(meshes, convex, uiCreated, uiSkipped).Succeeded()))
      return;

    EZ_TEST_INT(uiCreated, 3);
    EZ_TEST_INT(uiSkipped, 0);
  }

  // A guid that is not a mesh asset is a skip, not a failure - a selection can hold anything.
  {
    ezHybridArray<ezUuid, 2> mixed;
    mixed.PushBack(ezUuid::MakeUuid());

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshColliders(mixed, options, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT(uiCreated, 0);
    EZ_TEST_INT(uiSkipped, 1);
  }

  // A path in the options is meant for a single collider and must not make every mesh write to it.
  {
    ezMeshColliderOptions withPath = options;
    withPath.m_sColliderPath = expectedColliders[0];

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshColliders(meshes, withPath, uiCreated, uiSkipped).Succeeded());
    EZ_TEST_INT_MSG(uiCreated, 0, "Everything already exists, so nothing may be created.");
    EZ_TEST_INT(uiSkipped, 3);
  }
}

void ezEditorMeshColliderTest::SharedSourceFile()
{
  // Two mesh assets importing different sub-meshes out of one model file, as is common for asset
  // packs. Their colliders both depend on that file, so the file alone cannot tell them apart.
  const ezString sSource = MakePrivateSourceMesh("ColliderShared");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  ezVariantDictionary propsA;
  propsA.Insert("MeshIncludeTags", ezVariant(ezString("part_a")));

  ezVariantDictionary propsB;
  propsB.Insert("MeshIncludeTags", ezVariant(ezString("part_b")));

  const ezUuid meshA = CreateMeshAsset("MeshCollider/SharedA.ezMeshAsset", sSource, &propsA);
  const ezUuid meshB = CreateMeshAsset("MeshCollider/SharedB.ezMeshAsset", sSource, &propsB);
  if (!EZ_TEST_BOOL(meshA.IsValid() && meshB.IsValid()))
    return;

  // give A a collider, B none
  ezString sColliderA;
  {
    ezMeshColliderSource source;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshA, source).Succeeded()))
      return;

    sColliderA = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sColliderA;
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;
  }

  ProcessEvents(10);
  ezAssetCurator::GetSingleton()->MainThreadTick(true);

  {
    ezMeshColliderSource source;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshA, source).Succeeded()))
      return;

    EZ_TEST_BOOL_MSG(source.GetExisting(ezMeshColliderKind::TriangleMesh).IsValid(), "The mesh the collider was built from has to find it.");
  }

  {
    ezMeshColliderSource source;
    if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshB, source).Succeeded()))
      return;

    EZ_TEST_BOOL_MSG(!source.GetExisting(ezMeshColliderKind::TriangleMesh).IsValid(), "A collider for a different sub-mesh must not be picked up.");
  }
}

void ezEditorMeshColliderTest::Surface()
{
  const ezString sSource = MakePrivateSourceMesh("ColliderSurface");
  if (!EZ_TEST_BOOL(!sSource.IsEmpty()))
    return;

  const ezUuid meshGuid = CreateMeshAsset("MeshCollider/Surface.ezMeshAsset", sSource);
  if (!EZ_TEST_BOOL(meshGuid.IsValid()))
    return;

  ezMeshColliderSource source;
  if (!EZ_TEST_BOOL(ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, source).Succeeded()))
    return;

  // The surface is stored as an asset reference, which is a guid. Any guid does, it just has to
  // arrive in the document unchanged.
  ezStringBuilder sSurface;
  ezConversionUtils::ToString(ezUuid::MakeUuid(), sSurface);

  {
    const ezString sPath = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::ConvexHull);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = ezMeshColliderKind::ConvexHull;
    options.m_sSurface = sSurface;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    EZ_TEST_STRING(ReadColliderProperty(sPath, "Surface").ConvertTo<ezString>(), sSurface);
  }

  // A triangle mesh gets one surface per material slot of the model when it is transformed, so the
  // single "Surface" property is not its own and must be left alone.
  {
    const ezString sPath = ezMeshColliderCreator::SuggestColliderPath(source, ezMeshColliderKind::TriangleMesh);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = ezMeshColliderKind::TriangleMesh;
    options.m_sSurface = sSurface;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(source, options).Succeeded()))
      return;

    EZ_TEST_BOOL_MSG(ReadColliderProperty(sPath, "Surface").ConvertTo<ezString>().IsEmpty(), "A triangle mesh has no single surface.");
  }

  // no surface chosen has to leave the property alone rather than writing an empty reference
  {
    ezMeshColliderSource fresh;
    ezMeshColliderCreator::GatherMeshColliderSource(meshGuid, fresh).AssertSuccess();

    const ezString sPath = ezMeshColliderCreator::SuggestColliderPath(fresh, ezMeshColliderKind::ConvexHull);

    ezMeshColliderOptions options;
    options.m_sColliderPath = sPath;
    options.m_Kind = ezMeshColliderKind::ConvexHull;
    options.m_bOpenAfterCreate = false;

    if (!EZ_TEST_BOOL(ezMeshColliderCreator::CreateMeshCollider(fresh, options).Succeeded()))
      return;

    EZ_TEST_BOOL(ReadColliderProperty(sPath, "Surface").ConvertTo<ezString>().IsEmpty());
  }
}
