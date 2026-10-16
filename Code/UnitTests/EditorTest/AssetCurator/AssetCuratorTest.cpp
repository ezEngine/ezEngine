#include <EditorTest/EditorTestPCH.h>

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/AssetCurator/AssetCuratorTest.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorAssetCuratorTest s_EditorAssetCuratorTest;

const char* ezEditorAssetCuratorTest::GetTestName() const
{
  return "Asset Curator Tests";
}

void ezEditorAssetCuratorTest::SetupSubTests()
{
  AddSubTest("Package Dependency Change", SubTests::ST_PackageDependencyChange);
  AddSubTest("Package Dependency Broken", SubTests::ST_PackageDependencyBroken);
  AddSubTest("Error Clears When Fixed", SubTests::ST_ErrorClearsWhenFixed);
  AddSubTest("Repeated Transform Error", SubTests::ST_RepeatedTransformError);
  AddSubTest("Circular Package Dependencies", SubTests::ST_CircularPackageDependencies);
  AddSubTest("Circular Transform Dependencies", SubTests::ST_CircularTransformDependencies);
  AddSubTest("Mixed Dependency Cycle", SubTests::ST_MixedDependencyCycle);
  AddSubTest("Collection Unknown Entry", SubTests::ST_CollectionUnknownEntry);
  AddSubTest("Collection Scene Cycle", SubTests::ST_CollectionSceneCycle);
  AddSubTest("Broken Asset In Package Cycle", SubTests::ST_BrokenAssetInPackageCycle);
}

ezResult ezEditorAssetCuratorTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorAssetCuratorTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorAssetCuratorTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_PackageDependencyChange:
      PackageDependencyChange();
      break;
    case SubTests::ST_PackageDependencyBroken:
      PackageDependencyBroken();
      break;
    case SubTests::ST_ErrorClearsWhenFixed:
      ErrorClearsWhenFixed();
      break;
    case SubTests::ST_RepeatedTransformError:
      RepeatedTransformError();
      break;
    case SubTests::ST_CircularPackageDependencies:
      CircularPackageDependencies();
      break;
    case SubTests::ST_CircularTransformDependencies:
      CircularTransformDependencies();
      break;
    case SubTests::ST_MixedDependencyCycle:
      MixedDependencyCycle();
      break;
    case SubTests::ST_CollectionUnknownEntry:
      CollectionUnknownEntry();
      break;
    case SubTests::ST_CollectionSceneCycle:
      CollectionSceneCycle();
      break;
    case SubTests::ST_BrokenAssetInPackageCycle:
      BrokenAssetInPackageCycle();
      break;
  }

  CloseAllAssets();
  return ezTestAppRun::Quit;
}

//////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////

ezAssetDocument* ezEditorAssetCuratorTest::CreateAsset(ezStringView sFileName, bool bRequestWindow)
{
  ezStringBuilder sPath = m_sProjectPath;
  sPath.AppendPath(sFileName);

  ezAssetDocument* pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sPath, bRequestWindow ? ezDocumentFlags::RequestWindow : ezDocumentFlags::None));
  EZ_TEST_BOOL_MSG(pDoc != nullptr, "Failed to create '%s'", sPath.GetData());

  if (pDoc)
  {
    m_Assets.PushBack(pDoc);
    ProcessEvents();
  }

  return pDoc;
}

void ezEditorAssetCuratorTest::CloseAsset(ezAssetDocument* pDoc)
{
  m_Assets.RemoveAndCopy(pDoc);
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
  ProcessEvents();
}

void ezEditorAssetCuratorTest::CloseAllAssets()
{
  for (ezAssetDocument* pDoc : m_Assets)
  {
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  m_Assets.Clear();
  ProcessEvents();
}

void ezEditorAssetCuratorTest::SetProperty(ezAssetDocument* pDoc, ezStringView sProperty, const ezVariant& value)
{
  ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];

  pAcc->StartTransaction("Set Property");
  EZ_TEST_STATUS(pAcc->SetValueByName(pRoot, sProperty, value));
  pAcc->FinishTransaction();
}

void ezEditorAssetCuratorTest::AddCollectionEntry(ezAssetDocument* pDoc, ezStringView sAsset)
{
  ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const ezDocumentObject* pRoot = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];

  pAcc->StartTransaction("Add Entry");
  ezUuid entryGuid;
  EZ_TEST_STATUS(pAcc->AddObjectByName(pRoot, "Entries", -1, ezRTTI::FindTypeByName("ezCollectionAssetEntry"), entryGuid));
  EZ_TEST_STATUS(pAcc->SetValueByName(pAcc->GetObject(entryGuid), "Asset", sAsset));
  pAcc->FinishTransaction();
}

void ezEditorAssetCuratorTest::SaveAsset(ezAssetDocument* pDoc)
{
  // ezFileSystemModel detects modified files only by their modification time. Saving the same file twice within the resolution of
  // the file system time stamps would go unnoticed, so make sure that the time stamp of this save differs from the previous one.
  ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(20));

  EZ_TEST_BOOL(pDoc->SaveDocument().Succeeded());
  RunFrameTasks();
}

void ezEditorAssetCuratorTest::RunFrameTasks()
{
  ezTaskSystem::FinishFrameTasks();
  ProcessEvents();
}

ezTransformStatus ezEditorAssetCuratorTest::TransformAsset(const ezUuid& assetGuid)
{
  return ezAssetCurator::GetSingleton()->TransformAsset(assetGuid, ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);
}

ezAssetInfo::TransformState ezEditorAssetCuratorTest::UpdateState(const ezUuid& assetGuid, bool bForce)
{
  ezUInt64 uiAssetHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezUInt64 uiPackageHash = 0;
  return ezAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, nullptr, nullptr, uiAssetHash, uiThumbHash, uiPackageHash, bForce);
}

ezEditorAssetCuratorTest::AssetSnapshot ezEditorAssetCuratorTest::GetSnapshot(const ezUuid& assetGuid)
{
  AssetSnapshot res;

  const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!EZ_TEST_BOOL(pSubAsset.isValid()))
    return res;

  const ezAssetInfo* pInfo = pSubAsset->m_pAssetInfo;
  res.m_bValid = true;
  res.m_State = pInfo->m_TransformState;
  res.m_uiAssetHash = pInfo->m_AssetHash;
  res.m_uiThumbHash = pInfo->m_ThumbHash;
  res.m_uiPackageHash = pInfo->m_PackageHash;
  res.m_uiLastStateUpdate = pInfo->m_LastStateUpdate;
  return res;
}

ezString ezEditorAssetCuratorTest::ToString(const ezUuid& guid)
{
  ezStringBuilder sGuid;
  return ezConversionUtils::ToString(guid, sGuid);
}

//////////////////////////////////////////////////////////////////////////
// Sub-Tests
//////////////////////////////////////////////////////////////////////////

void ezEditorAssetCuratorTest::PackageDependencyChange()
{
  // Collection entries are package dependencies. Package dependencies exist so that changing an asset does not require
  // re-transforming (or even re-hashing) the assets that only reference it.
  ezAssetDocument* pTarget = CreateAsset("PackageChangeTarget.ezCollectionAsset");
  ezAssetDocument* pOther = CreateAsset("PackageChangeOther.ezCollectionAsset");
  ezAssetDocument* pUser = CreateAsset("PackageChangeUser.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pTarget && pOther && pUser))
    return;

  SaveAsset(pTarget);
  SaveAsset(pOther);
  AddCollectionEntry(pUser, ToString(pTarget->GetGuid()));
  SaveAsset(pUser);

  EZ_TEST_BOOL(TransformAsset(pUser->GetGuid()).Succeeded());
  EZ_TEST_BOOL(TransformAsset(pTarget->GetGuid()).Succeeded());
  EZ_TEST_BOOL(UpdateState(pUser->GetGuid()) == ezAssetInfo::TransformState::UpToDate);

  // A transform invalidates the asset, whether its hashes are computed again by the time the snapshot is taken depends on the
  // curator's background update task. UpdateState makes sure they are.
  EZ_TEST_BOOL(UpdateState(pTarget->GetGuid()) == ezAssetInfo::TransformState::UpToDate);
  const AssetSnapshot userBefore = GetSnapshot(pUser->GetGuid());
  const AssetSnapshot targetBefore = GetSnapshot(pTarget->GetGuid());

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Modify the referenced asset")
  {
    AddCollectionEntry(pTarget, ToString(pOther->GetGuid()));
    SaveAsset(pTarget);
    EZ_TEST_BOOL(TransformAsset(pTarget->GetGuid()).Succeeded());
    EZ_TEST_BOOL(UpdateState(pTarget->GetGuid()) == ezAssetInfo::TransformState::UpToDate);

    // The modified asset itself was invalidated, this is what the check below relies on.
    const AssetSnapshot targetAfter = GetSnapshot(pTarget->GetGuid());
    EZ_TEST_BOOL(targetAfter.m_uiLastStateUpdate != targetBefore.m_uiLastStateUpdate);
    EZ_TEST_BOOL(targetAfter.m_uiAssetHash != targetBefore.m_uiAssetHash);

    // The referencing asset was not invalidated.
    const AssetSnapshot userAfter = GetSnapshot(pUser->GetGuid());
    EZ_TEST_INT(userAfter.m_uiLastStateUpdate, userBefore.m_uiLastStateUpdate);
    EZ_TEST_INT(userAfter.m_uiAssetHash, userBefore.m_uiAssetHash);
    EZ_TEST_BOOL(userAfter.m_State == ezAssetInfo::TransformState::UpToDate);
  }
}

void ezEditorAssetCuratorTest::PackageDependencyBroken()
{
  // An asset can't be hashed when a package dependency can't be hashed. Assets are not invalidated when a package dependency changes,
  // but they have to be when it becomes broken or gets fixed, otherwise they keep showing the old state.
  ezTestLogInterface log;
  log.IgnoreMessage("PackageBroken");
  log.IgnoreMessage("Failed to hash dependency asset");
  log.IgnoreMessage("{ 01234567-89ab-cdef-0123-456789abcdef }");
  ezTestLogSystemScope logSystemScope(&log, true);

  ezAssetDocument* pTarget = CreateAsset("PackageBrokenTarget.ezCollectionAsset");
  ezAssetDocument* pUser = CreateAsset("PackageBrokenUser.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pTarget && pUser))
    return;

  ezObjectAccessorBase* pAcc = pTarget->GetObjectAccessor();
  const ezDocumentObject* pTargetRoot = pTarget->GetObjectManager()->GetRootObject()->GetChildren()[0];

  SaveAsset(pTarget);
  AddCollectionEntry(pUser, ToString(pTarget->GetGuid()));
  SaveAsset(pUser);
  EZ_TEST_BOOL(TransformAsset(pTarget->GetGuid()).Succeeded());
  EZ_TEST_BOOL(TransformAsset(pUser->GetGuid()).Succeeded());
  EZ_TEST_BOOL(UpdateState(pUser->GetGuid()) == ezAssetInfo::TransformState::UpToDate);

  for (ezUInt32 i = 0; i < 2; ++i)
  {
    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Break the referenced asset")
    {
      AddCollectionEntry(pTarget, "{ 01234567-89ab-cdef-0123-456789abcdef }");
      SaveAsset(pTarget);

      EZ_TEST_BOOL(UpdateState(pTarget->GetGuid()) == ezAssetInfo::TransformState::MissingPackageDependency);
      const ezAssetInfo::TransformState state = UpdateState(pUser->GetGuid());
      EZ_TEST_BOOL_MSG(state == ezAssetInfo::TransformState::MissingPackageDependency, "State: %d", (int)state);
    }

    EZ_TEST_BLOCK(ezTestBlock::Enabled, "Fix the referenced asset")
    {
      pAcc->StartTransaction("Remove Entry");
      EZ_TEST_STATUS(pAcc->RemoveObject(pAcc->GetChildObjectByName(pTargetRoot, "Entries", 0)));
      pAcc->FinishTransaction();
      SaveAsset(pTarget);
      EZ_TEST_BOOL(TransformAsset(pTarget->GetGuid()).Succeeded());

      const ezAssetInfo::TransformState targetState = UpdateState(pTarget->GetGuid());
      EZ_TEST_BOOL_MSG(targetState == ezAssetInfo::TransformState::UpToDate, "State: %d", (int)targetState);
      const ezAssetInfo::TransformState state = UpdateState(pUser->GetGuid());
      EZ_TEST_BOOL_MSG(state == ezAssetInfo::TransformState::UpToDate, "State: %d", (int)state);
    }
  }
}

void ezEditorAssetCuratorTest::ErrorClearsWhenFixed()
{
  ezTestLogInterface log;
  log.IgnoreMessage("ErrorClearsCollection.ezCollectionAsset");
  log.IgnoreMessage("ErrorClearsMesh.ezMeshAsset");
  log.IgnoreMessage("Meshes/Missing.obj");
  ezTestLogSystemScope logSystemScope(&log, true);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform error")
  {
    ezAssetDocument* pDoc = CreateAsset("ErrorClearsCollection.ezCollectionAsset");
    ezAssetDocument* pValid = CreateAsset("ErrorClearsValid.ezCollectionAsset");
    if (!EZ_TEST_BOOL(pDoc && pValid))
      return;

    SaveAsset(pValid);

    // A file that exists but is not an asset can't be part of a collection.
    AddCollectionEntry(pDoc, "Meshes/Cube.obj");
    SaveAsset(pDoc);
    EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Failed());
    EZ_TEST_BOOL(GetSnapshot(pDoc->GetGuid()).m_State == ezAssetInfo::TransformState::TransformError);

    // The error is kept until something changes on disk.
    EZ_TEST_BOOL(UpdateState(pDoc->GetGuid()) == ezAssetInfo::TransformState::TransformError);

    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    const ezDocumentObject* pEntry = pAcc->GetChildObjectByName(pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0], "Entries", 0);
    pAcc->StartTransaction("Fix Entry");
    EZ_TEST_STATUS(pAcc->SetValueByName(pEntry, "Asset", ToString(pValid->GetGuid())));
    pAcc->FinishTransaction();
    SaveAsset(pDoc);

    const ezAssetInfo::TransformState state = UpdateState(pDoc->GetGuid());
    EZ_TEST_BOOL_MSG(state == ezAssetInfo::TransformState::UpToDate || state == ezAssetInfo::TransformState::NeedsTransform, "State: %d", (int)state);

    EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Succeeded());
    EZ_TEST_BOOL(UpdateState(pDoc->GetGuid()) == ezAssetInfo::TransformState::UpToDate);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Missing transform dependency")
  {
    ezAssetDocument* pDoc = CreateAsset("ErrorClearsMesh.ezMeshAsset", true);
    if (!EZ_TEST_BOOL(pDoc))
      return;

    SetProperty(pDoc, "MeshFile", "Meshes/Missing.obj");
    SaveAsset(pDoc);
    EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Failed());
    EZ_TEST_BOOL(GetSnapshot(pDoc->GetGuid()).m_State == ezAssetInfo::TransformState::MissingTransformDependency);

    SetProperty(pDoc, "MeshFile", "Meshes/Cube.obj");
    SaveAsset(pDoc);

    const ezAssetInfo::TransformState state = UpdateState(pDoc->GetGuid());
    EZ_TEST_BOOL_MSG(state != ezAssetInfo::TransformState::MissingTransformDependency && state != ezAssetInfo::TransformState::TransformError, "State: %d", (int)state);

    // The first transform imports the mesh's materials, which modifies the asset, so it needs another transform afterwards.
    EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Succeeded());
    EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Succeeded());
    const ezAssetInfo::TransformState state2 = UpdateState(pDoc->GetGuid());
    EZ_TEST_BOOL_MSG(state2 == ezAssetInfo::TransformState::UpToDate, "State: %d", (int)state2);
  }
}

void ezEditorAssetCuratorTest::RepeatedTransformError()
{
  // Saving the collection also transforms it, which fails and logs the same error.
  ezTestLogInterface log;
  log.IgnoreMessage("FailingCollection.ezCollectionAsset");
  ezTestLogSystemScope logSystemScope(&log, true);

  ezAssetDocument* pDoc = CreateAsset("FailingCollection.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pDoc != nullptr))
    return;

  // A file that exists but is not an asset can't be part of a collection, so the transform fails, even though no dependency is missing.
  AddCollectionEntry(pDoc, "Meshes/Cube.obj");
  SaveAsset(pDoc);

  // Recording the error used to leave the asset with the zero hashes of the invalidation that follows every manual transform,
  // so every transform after the first one failed with "Computing the hash for this asset or any dependency failed".
  for (ezUInt32 i = 0; i < 3; ++i)
  {
    const ezTransformStatus res = TransformAsset(pDoc->GetGuid());
    EZ_TEST_BOOL(res.Failed());
    EZ_TEST_BOOL_MSG(res.m_sMessage.FindSubString("Asset in Collection is unknown") != nullptr, "Transform %u failed with: %s", i, res.m_sMessage.GetData());
  }

  const AssetSnapshot snapshot = GetSnapshot(pDoc->GetGuid());
  EZ_TEST_BOOL(snapshot.m_State == ezAssetInfo::TransformState::TransformError);
  EZ_TEST_BOOL(snapshot.m_uiAssetHash != 0);
}

void ezEditorAssetCuratorTest::CircularPackageDependencies()
{
  // Hashing an asset in a package dependency cycle used to recurse endlessly.
  auto TestCycle = [&](ezArrayPtr<ezAssetDocument*> cycle)
  {
    for (ezUInt32 i = 0; i < cycle.GetCount(); ++i)
    {
      AddCollectionEntry(cycle[i], ToString(cycle[(i + 1) % cycle.GetCount()]->GetGuid()));
    }

    for (ezAssetDocument* pDoc : cycle)
    {
      SaveAsset(pDoc);
    }

    for (ezAssetDocument* pDoc : cycle)
    {
      EZ_TEST_BOOL(TransformAsset(pDoc->GetGuid()).Succeeded());
      EZ_TEST_BOOL(UpdateState(pDoc->GetGuid(), true) == ezAssetInfo::TransformState::UpToDate);
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Two collections")
  {
    ezAssetDocument* docs[] = {CreateAsset("CircularA.ezCollectionAsset"), CreateAsset("CircularB.ezCollectionAsset")};
    if (EZ_TEST_BOOL(docs[0] && docs[1]))
      TestCycle(docs);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Three collections")
  {
    ezAssetDocument* docs[] = {CreateAsset("CircularP.ezCollectionAsset"), CreateAsset("CircularQ.ezCollectionAsset"), CreateAsset("CircularR.ezCollectionAsset")};
    if (EZ_TEST_BOOL(docs[0] && docs[1] && docs[2]))
      TestCycle(docs);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Self reference")
  {
    ezAssetDocument* docs[] = {CreateAsset("CircularSelf.ezCollectionAsset")};
    if (EZ_TEST_BOOL(docs[0]))
      TestCycle(docs);
  }
}

void ezEditorAssetCuratorTest::CircularTransformDependencies()
{
  // Cycles through transform dependencies can't be transformed, the curator has to detect them and report them as errors.
  ezTestLogInterface log;
  log.IgnoreMessage("CircularMaterialA.ezMaterialAsset");
  log.IgnoreMessage("CircularMaterialB.ezMaterialAsset");
  ezTestLogSystemScope logSystemScope(&log, true);

  ezAssetDocument* pA = CreateAsset("CircularMaterialA.ezMaterialAsset");
  ezAssetDocument* pB = CreateAsset("CircularMaterialB.ezMaterialAsset");
  if (!EZ_TEST_BOOL(pA && pB))
    return;

  // Hashing an asset whose dependency is circular logs an error with the dependency's GUID. Whether that happens depends on whether
  // the curator's background update task hashed it first, which doesn't log.
  log.IgnoreMessage(ToString(pA->GetGuid()));
  log.IgnoreMessage(ToString(pB->GetGuid()));

  auto IsErrorState = [](ezAssetInfo::TransformState state)
  {
    return state == ezAssetInfo::TransformState::CircularDependency || state == ezAssetInfo::TransformState::MissingTransformDependency;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create cycle")
  {
    SetProperty(pA, "BaseMaterial", ToString(pB->GetGuid()));
    SaveAsset(pA);
    SetProperty(pB, "BaseMaterial", ToString(pA->GetGuid()));
    SaveAsset(pB);

    for (ezAssetDocument* pDoc : {pA, pB})
    {
      const ezTransformStatus res = TransformAsset(pDoc->GetGuid());
      EZ_TEST_BOOL(res.Failed());

      const ezAssetInfo::TransformState state = UpdateState(pDoc->GetGuid());
      EZ_TEST_BOOL_MSG(IsErrorState(state), "State: %d", (int)state);
    }

    // The asset that closed the cycle is the one that is known to be circular. The other one can't be hashed, because its
    // dependency is circular. The base material is also a package dependency, but the missing transform dependency is what matters.
    EZ_TEST_BOOL(GetSnapshot(pB->GetGuid()).m_State == ezAssetInfo::TransformState::CircularDependency);
    EZ_TEST_BOOL(GetSnapshot(pA->GetGuid()).m_State == ezAssetInfo::TransformState::MissingTransformDependency);

    const ezTransformStatus res = TransformAsset(pA->GetGuid());
    // Transforming A transforms its dependencies first, so the failure points at B.
    EZ_TEST_BOOL_MSG(res.m_sMessage.FindSubString("Circular dependency for asset") != nullptr && res.m_sMessage.FindSubString("CircularMaterialB") != nullptr, "Transform failed with: %s", res.m_sMessage.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Break cycle")
  {
    SetProperty(pB, "BaseMaterial", "");
    SaveAsset(pB);

    for (ezAssetDocument* pDoc : {pB, pA})
    {
      const ezTransformStatus res = TransformAsset(pDoc->GetGuid());
      EZ_TEST_BOOL_MSG(res.Succeeded(), "Transform failed: %s", res.m_sMessage.GetData());
      EZ_TEST_BOOL(UpdateState(pDoc->GetGuid()) == ezAssetInfo::TransformState::UpToDate);
    }
  }
}

void ezEditorAssetCuratorTest::MixedDependencyCycle()
{
  // A has a package dependency on B, B has a transform dependency on A. Only cycles through transform and thumbnail dependencies
  // are rejected, so both assets have to be transformable.
  // No asset type allows such a setup with plain property values, so B's transform dependency is added to the document header on disk,
  // which is where the curator reads the dependencies from.
  ezAssetDocument* pA = CreateAsset("MixedCycleA.ezCollectionAsset");
  ezAssetDocument* pB = CreateAsset("MixedCycleB.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pA && pB))
    return;

  const ezUuid guidA = pA->GetGuid();
  const ezUuid guidB = pB->GetGuid();
  const ezString sPathB = pB->GetDocumentPath();

  AddCollectionEntry(pA, ToString(guidB));
  SaveAsset(pA);
  SaveAsset(pB);
  CloseAsset(pB);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Add transform dependency")
  {
    ezStringBuilder sContent;
    {
      ezOSFile file;
      if (!EZ_TEST_RESULT(file.Open(sPathB, ezFileOpenMode::Read)))
        return;

      ezDynamicArray<ezUInt8> data;
      file.ReadAll(data);
      sContent = ezStringView(reinterpret_cast<const char*>(data.GetData()), data.GetCount());
    }

    ezStringBuilder sDeps;
    sDeps.Append("VarArray %Dependencies\n\t\t{\n\t\t\ts{\"", ToString(guidA), "\"}\n\t\t}");
    if (!EZ_TEST_BOOL(sContent.FindSubString("VarArray %Dependencies{}") != nullptr))
      return;
    sContent.ReplaceFirst("VarArray %Dependencies{}", sDeps);

    {
      ezOSFile file;
      if (!EZ_TEST_RESULT(file.Open(sPathB, ezFileOpenMode::Write)))
        return;
      EZ_TEST_RESULT(file.Write(sContent.GetData(), sContent.GetElementCount()));
    }

    ezAssetCurator::GetSingleton()->NotifyOfFileChange(sPathB);
    ezAssetCurator::GetSingleton()->MainThreadTick(false);
    RunFrameTasks();

    const ezAssetCurator::ezLockedSubAsset pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guidB);
    if (EZ_TEST_BOOL(pSubAsset.isValid()))
    {
      EZ_TEST_BOOL(pSubAsset->m_pAssetInfo->m_Info->m_TransformDependencies.Contains(ToString(guidA)));
      EZ_TEST_BOOL(pSubAsset->m_pAssetInfo->m_CircularDependencies.IsEmpty());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Hashes don't depend on the entry point")
  {
    // Hashing starts at A, B is hashed as A's package dependency.
    EZ_TEST_BOOL(UpdateState(guidA, true) != ezAssetInfo::TransformState::Unknown);
    const AssetSnapshot a1 = GetSnapshot(guidA);
    const AssetSnapshot b1 = GetSnapshot(guidB);

    // Hashing starts at B, A is hashed as B's transform dependency.
    EZ_TEST_BOOL(UpdateState(guidB, true) != ezAssetInfo::TransformState::Unknown);
    const AssetSnapshot a2 = GetSnapshot(guidA);
    const AssetSnapshot b2 = GetSnapshot(guidB);

    EZ_TEST_INT(a1.m_uiAssetHash, a2.m_uiAssetHash);
    EZ_TEST_INT(a1.m_uiThumbHash, a2.m_uiThumbHash);
    EZ_TEST_INT(b1.m_uiAssetHash, b2.m_uiAssetHash);
    EZ_TEST_INT(b1.m_uiThumbHash, b2.m_uiThumbHash);
    EZ_TEST_BOOL(a1.m_uiAssetHash != 0 && b1.m_uiAssetHash != 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    for (const ezUuid& guid : {guidA, guidB})
    {
      const ezTransformStatus res = TransformAsset(guid);
      EZ_TEST_BOOL_MSG(res.Succeeded(), "Transform failed: %s", res.m_sMessage.GetData());
      EZ_TEST_BOOL(UpdateState(guid) == ezAssetInfo::TransformState::UpToDate);
    }
  }
}

void ezEditorAssetCuratorTest::CollectionUnknownEntry()
{
  ezTestLogInterface log;
  log.IgnoreMessage("UnknownEntryC.ezCollectionAsset");
  log.IgnoreMessage("UnknownEntryD.ezCollectionAsset");
  ezTestLogSystemScope logSystemScope(&log, true);

  ezAssetDocument* pC = CreateAsset("UnknownEntryC.ezCollectionAsset");
  ezAssetDocument* pD = CreateAsset("UnknownEntryD.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pC && pD))
    return;

  // A file that exists but is not an asset can't be part of a collection.
  AddCollectionEntry(pD, "Meshes/Cube.obj");
  SaveAsset(pD);

  // C reaches the file through D first, which must not hide that C's own entry is unknown.
  AddCollectionEntry(pC, ToString(pD->GetGuid()));
  AddCollectionEntry(pC, "Meshes/Cube.obj");
  SaveAsset(pC);

  const ezTransformStatus res = TransformAsset(pC->GetGuid());
  EZ_TEST_BOOL(res.Failed());
  EZ_TEST_BOOL_MSG(res.m_sMessage.FindSubString("Asset in Collection is unknown: 'Meshes/Cube.obj'") != nullptr, "Transform failed with: %s", res.m_sMessage.GetData());
}

void ezEditorAssetCuratorTest::CollectionSceneCycle()
{
  // Two scenes that transition into each other. A collection with one scene contains everything both scenes reference.
  ezAssetDocument* pMesh = CreateAsset("SceneCycleMesh.ezMeshAsset", true);
  ezScene2Document* pSceneA = static_cast<ezScene2Document*>(CreateAsset("SceneCycleA.ezScene", true));
  ezScene2Document* pSceneB = static_cast<ezScene2Document*>(CreateAsset("SceneCycleB.ezScene", true));
  ezAssetDocument* pCollection = CreateAsset("SceneCycleCollection.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pMesh && pSceneA && pSceneB && pCollection))
    return;

  SetProperty(pMesh, "MeshFile", "Meshes/Cube.obj");
  SaveAsset(pMesh);

  auto AddComponent = [&](ezScene2Document* pScene, ezStringView sType, ezStringView sProperty, ezStringView sValue)
  {
    const ezDocumentObject* pObject = CreateGameObject(pScene);

    ezObjectAccessorBase* pAcc = pScene->GetObjectAccessor();
    pAcc->StartTransaction("Add Component");
    ezUuid compGuid;
    EZ_TEST_STATUS(pAcc->AddObjectByName(pObject, "Components", -1, ezRTTI::FindTypeByName(sType), compGuid));
    EZ_TEST_STATUS(pAcc->SetValueByName(pAcc->GetObject(compGuid), sProperty, sValue));
    pAcc->FinishTransaction();
  };

  AddComponent(pSceneA, "ezSceneTransitionComponent", "TargetScene", ToString(pSceneB->GetGuid()));
  AddComponent(pSceneB, "ezSceneTransitionComponent", "TargetScene", ToString(pSceneA->GetGuid()));
  AddComponent(pSceneB, "ezMeshComponent", "Mesh", ToString(pMesh->GetGuid()));
  SaveAsset(pSceneA);
  SaveAsset(pSceneB);

  AddCollectionEntry(pCollection, ToString(pSceneA->GetGuid()));
  SaveAsset(pCollection);

  const ezTransformStatus res = TransformAsset(pCollection->GetGuid());
  if (!EZ_TEST_BOOL_MSG(res.Succeeded(), "Transform failed: %s", res.m_sMessage.GetData()))
    return;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Verify collection")
  {
    const ezString sOutput = pCollection->GetAssetDocumentManager()->GetAbsoluteOutputFileName(pCollection->GetAssetDocumentTypeDescriptor(), pCollection->GetDocumentPath(), "");

    ezFileReader file;
    if (!EZ_TEST_RESULT(file.Open(sOutput)))
      return;

    ezAssetFileHeader header;
    EZ_TEST_RESULT(header.Read(file));

    ezCollectionResourceDescriptor desc;
    desc.Load(file);

    // Scenes are not part of the collection, but everything they reference is.
    bool bFoundMesh = false;
    for (const ezCollectionEntry& entry : desc.m_Resources)
    {
      EZ_TEST_BOOL(entry.m_sResourceID != ToString(pSceneA->GetGuid()));
      EZ_TEST_BOOL(entry.m_sResourceID != ToString(pSceneB->GetGuid()));
      bFoundMesh |= entry.m_sResourceID == ToString(pMesh->GetGuid());
    }

    EZ_TEST_BOOL(bFoundMesh);
  }
}

void ezEditorAssetCuratorTest::BrokenAssetInPackageCycle()
{
  // A and B reference each other, A also references an asset that does not exist.
  // While hashing, the cycle is broken by assuming that an asset that is currently being hashed is fine. So when hashing starts at A,
  // B considers A to be fine, when it starts at B, B sees A's missing dependency. B's state therefore depends on the order of hashing.
  // This is accepted: only the state shown for B differs, A is always reported as broken, which is what blocks exporting the project.
  ezAssetDocument* pA = CreateAsset("BrokenCycleA.ezCollectionAsset");
  ezAssetDocument* pB = CreateAsset("BrokenCycleB.ezCollectionAsset");
  if (!EZ_TEST_BOOL(pA && pB))
    return;

  ezTestLogInterface log;
  log.IgnoreMessage("BrokenCycle");
  log.IgnoreMessage("Failed to hash dependency asset");
  log.IgnoreMessage("{ 01234567-89ab-cdef-0123-456789abcdef }");
  ezTestLogSystemScope logSystemScope(&log, true);

  AddCollectionEntry(pA, ToString(pB->GetGuid()));
  AddCollectionEntry(pA, "{ 01234567-89ab-cdef-0123-456789abcdef }");
  AddCollectionEntry(pB, ToString(pA->GetGuid()));
  SaveAsset(pA);
  SaveAsset(pB);

  const ezAssetInfo::TransformState stateA1 = UpdateState(pA->GetGuid(), true);
  const ezAssetInfo::TransformState stateB1 = GetSnapshot(pB->GetGuid()).m_State;

  const ezAssetInfo::TransformState stateB2 = UpdateState(pB->GetGuid(), true);
  const ezAssetInfo::TransformState stateA2 = GetSnapshot(pA->GetGuid()).m_State;

  auto IsValidStateForB = [](ezAssetInfo::TransformState state)
  {
    return state == ezAssetInfo::TransformState::MissingPackageDependency || state == ezAssetInfo::TransformState::NeedsTransform || state == ezAssetInfo::TransformState::UpToDate;
  };

  EZ_TEST_BOOL(stateA1 == ezAssetInfo::TransformState::MissingPackageDependency);
  EZ_TEST_BOOL(stateA2 == ezAssetInfo::TransformState::MissingPackageDependency);
  EZ_TEST_BOOL_MSG(IsValidStateForB(stateB1), "State: %d", (int)stateB1);
  EZ_TEST_BOOL_MSG(IsValidStateForB(stateB2), "State: %d", (int)stateB2);

  // Transforming A has to report the missing dependency, no matter in which state B is.
  const ezTransformStatus res = TransformAsset(pA->GetGuid());
  EZ_TEST_BOOL(res.Failed());
}
