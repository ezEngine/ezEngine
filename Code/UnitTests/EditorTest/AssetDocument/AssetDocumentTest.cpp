#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorTest/AssetDocument/AssetDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static ezEditorAssetDocumentTest s_EditorAssetDocumentTest;

const char* ezEditorAssetDocumentTest::GetTestName() const
{
  return "Asset Document Tests";
}

void ezEditorAssetDocumentTest::SetupSubTests()
{
  AddSubTest("Async Save", SubTests::ST_AsyncSave);
  AddSubTest("Save on Transform", SubTests::ST_SaveOnTransform);
  AddSubTest("File Operations", SubTests::ST_FileOperations);
}

ezResult ezEditorAssetDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return EZ_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezEditorAssetDocumentTest::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezEditorAssetDocumentTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_AsyncSave:
      AsyncSave();
      break;
    case SubTests::ST_SaveOnTransform:
      SaveOnTransform();
      break;
    case SubTests::ST_FileOperations:
      FileOperations();
      break;
  }
  return ezTestAppRun::Quit;
}

void ezEditorAssetDocumentTest::AsyncSave()
{
  ezAssetDocument* pDoc = nullptr;
  ezStringBuilder sName;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    sName = m_sProjectPath;
    sName.AppendPath("mesh.ezMeshAsset");
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(pDoc != nullptr))
      return;
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Save Document")
  {
    // Save doc twice in a row without processing messages and then close it.
    ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    ezInt32 iOrder = 0;
    ezTaskGroupID id = pDoc->SaveDocumentAsync(
      [&iOrder](ezDocument* pDoc, ezStatus res)
      {
        EZ_TEST_INT(iOrder, 0);
        iOrder = 1;
      },
      true);

    pAcc->StartTransaction("Edit Mesh");
    EZ_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    // Saving while another save is in progress should block. This ensures the correct state on disk.
    ezString sFile = pAcc->GetByName<ezString>(pMeshAsset, "MeshFile");
    ezTaskGroupID id2 = pDoc->SaveDocumentAsync([&iOrder](ezDocument* pDoc, ezStatus res)
      {
      EZ_TEST_INT(iOrder, 1);
      iOrder = 2; });

    // Closing the document should wait for the async save to finish.
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    EZ_TEST_INT(iOrder, 2);
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id));
    EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(id2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Verify State of Disk")
  {
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->OpenDocument(sName, ezDocumentFlags::None));
    ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    ezString sFile = pAcc->GetByName<ezString>(pMeshAsset, "MeshFile");
    EZ_TEST_STRING(sFile, "Meshes/Cube.obj");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}

void ezEditorAssetDocumentTest::SaveOnTransform()
{
  ezAssetDocument* pDoc = nullptr;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    ezStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh2.ezMeshAsset");
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    if (!EZ_TEST_BOOL(pDoc != nullptr))
      return;
    ProcessEvents();
  }

  ezObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
  const ezDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transform")
  {
    pAcc->StartTransaction("Edit Mesh");
    EZ_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();

    ezTransformStatus res = pDoc->SaveDocument();
    EZ_TEST_BOOL(res.Succeeded());

    // Transforming an asset in the background should fail and return NeedsImport as the asset needs to be modified which the background is not allowed to do, e.g. materials need to be created.
    res = ezAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::BackgroundProcessing);
    EZ_TEST_BOOL(res.m_Result == ezTransformResult::NeedsImport);

    // Transforming a mesh asset with a mesh reference will trigger the material import and update
    // the materials table which requires a save during transform.
    res = ezAssetCurator::GetSingleton()->TransformAsset(pDoc->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);
    EZ_TEST_BOOL(res.Succeeded());
    ProcessEvents();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Verify Transform")
  {
    // Transforming should have update the mesh asset with new material slots.
    ezInt32 iCount = 0;
    EZ_TEST_BOOL(pAcc->GetCountByName(pMeshAsset, "Materials", iCount).Succeeded());
    EZ_TEST_INT(iCount, 1);

    ezUuid subObject = pAcc->GetByName<ezUuid>(pMeshAsset, "Materials", (ezInt64)0);
    EZ_TEST_BOOL(subObject.IsValid());
    const ezDocumentObject* pSubObject = pAcc->GetObject(subObject);

    ezString sLabel = pAcc->GetByName<ezString>(pSubObject, "Label");
    EZ_TEST_STRING(sLabel, "initialShadingGroup");
  }
  pDoc->GetDocumentManager()->CloseDocument(pDoc);
}


void ezEditorAssetDocumentTest::FileOperations()
{
  struct AssetEvent
  {
    AssetEvent() = default;
    AssetEvent(ezStringView sAbsPath, ezUuid assetGuid, ezAssetCuratorEvent::Type type)
      : m_sAbsPath(sAbsPath)
      , m_AssetGuid(assetGuid)
      , m_Type(type)
    {
    }

    ezString m_sAbsPath;
    ezUuid m_AssetGuid;
    ezAssetCuratorEvent::Type m_Type;
  };

  class EventHandler : public ezRefCounted
  {
  public:
    void Init(const ezSharedPtr<EventHandler>& pStrongThis)
    {
      // Multi-threaded event subscription can outlive the target lifetime if we don't capture a string reference to it.
      m_FileID = ezFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler([pStrongThis](const ezFileChangedEvent& e)
        { pStrongThis->OnAssetFilesEvent(e); });
      m_AssetID = ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&EventHandler::OnAssetEvent, this));
    }

    void DeInit()
    {
      ezFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(m_FileID);
      ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(m_AssetID);
    }

    void OnAssetFilesEvent(const ezFileChangedEvent& e)
    {
      if (e.m_Path.GetAbsolutePath().GetFileExtension().IsEqual_NoCase("ezAidlt"_ezsv))
        return;

      if (m_FileID == 0)
        return;

      EZ_LOCK(m_EventMutex);
      m_FileEvents.PushBack(e);
    }

    void OnAssetEvent(const ezAssetCuratorEvent& e)
    {
      m_AssetEvents.PushBack({e.m_pInfo->m_pAssetInfo->m_Path, e.m_AssetGuid, e.m_Type});
    }



    ezMutex m_EventMutex;
    ezHybridArray<ezFileChangedEvent, 4> m_FileEvents;
    ezHybridArray<AssetEvent, 4> m_AssetEvents;

  private:
    ezEventSubscriptionID m_FileID = 0;
    ezEventSubscriptionID m_AssetID = 0;
  };

  ezSharedPtr<EventHandler> events = EZ_DEFAULT_NEW(EventHandler);
  events->Init(events);
  EZ_SCOPE_EXIT(events->DeInit());

  auto CompareFiles = [&](ezArrayPtr<ezFileChangedEvent> expectedFiles, ezArrayPtr<AssetEvent> expectedAssets)
  {
    EZ_LOCK(events->m_EventMutex);
    if (EZ_TEST_INT(expectedFiles.GetCount(), events->m_FileEvents.GetCount()))
    {
      for (ezUInt32 i = 0; i < expectedFiles.GetCount(); i++)
      {
        EZ_TEST_INT((int)expectedFiles[i].m_Type, (int)events->m_FileEvents[i].m_Type);
        EZ_TEST_STRING(expectedFiles[i].m_Path, events->m_FileEvents[i].m_Path);
        // Ignore stats
      }
    }

    if (EZ_TEST_INT(expectedAssets.GetCount(), events->m_AssetEvents.GetCount()))
    {
      for (ezUInt32 i = 0; i < expectedAssets.GetCount(); i++)
      {
        EZ_TEST_INT((int)expectedAssets[i].m_Type, (int)events->m_AssetEvents[i].m_Type);
        EZ_TEST_STRING(expectedAssets[i].m_sAbsPath, events->m_AssetEvents[i].m_sAbsPath);
        EZ_TEST_BOOL(expectedAssets[i].m_AssetGuid == events->m_AssetEvents[i].m_AssetGuid);
        // Ignore stats
      }
    }
    events->m_FileEvents.Clear();
    events->m_AssetEvents.Clear();
  };

  auto WaitForFileEvents = [&](ezUInt32 uiFileEventCount, ezUInt32 uiAssetEventCount)
  {
    constexpr ezUInt32 WAIT_LOOPS = 500;
    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ProcessEvents();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));

      EZ_LOCK(events->m_EventMutex);
      if (events->m_FileEvents.GetCount() == uiFileEventCount && events->m_AssetEvents.GetCount() == uiAssetEventCount)
        break;
    }
  };

  auto FlushEvents = [&]()
  {
    constexpr ezUInt32 WAIT_LOOPS = 60;
    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ProcessEvents();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));

      EZ_LOCK(events->m_EventMutex);
      if (!events->m_FileEvents.IsEmpty() || !events->m_AssetEvents.IsEmpty())
      {
        i = 0;
        events->m_FileEvents.Clear();
        events->m_AssetEvents.Clear();
      }
    }
  };

  auto CheckSubAsset = [](const ezAssetCurator::ezLockedSubAsset& subAsset, ezUuid documentGuid, ezString sAbsAssetPath)
  {
    if (EZ_TEST_BOOL(subAsset))
    {
      EZ_TEST_BOOL(subAsset->m_ExistanceState == ezAssetExistanceState::FileUnchanged);
      EZ_TEST_BOOL(subAsset->m_bMainAsset);
      EZ_TEST_BOOL(subAsset->m_Data.m_Guid == documentGuid);
      EZ_TEST_STRING(subAsset->m_Data.m_sSubAssetsDocumentTypeName, "Mesh");
      EZ_TEST_STRING(subAsset->m_Data.m_sName, "");
      EZ_TEST_BOOL(subAsset->m_pAssetInfo->m_ExistanceState == ezAssetExistanceState::FileUnchanged);
      EZ_TEST_BOOL(subAsset->m_pAssetInfo->m_TransformState == ezAssetInfo::NeedsTransform);
      EZ_TEST_BOOL(subAsset->m_pAssetInfo->m_Info->m_DocumentID == documentGuid);
      EZ_TEST_STRING(subAsset->m_pAssetInfo->m_Path, sAbsAssetPath);
    }
  };

  ezHybridArray<ezString, 4> rootFolders(ezFileSystemModel::GetSingleton()->GetDataDirectoryRoots());
  auto MakePath = [&](ezStringView sPath)
  {
    return ezDataDirPath(sPath, rootFolders);
  };

  // Wait for the asset curator to process all file events from opening the project.
  FlushEvents();

  ezAssetDocument* pDoc = nullptr;
  ezString sAbsAssetPath;
  ezUuid documentGuid;
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create Document")
  {
    ezStringBuilder sName = m_sProjectPath;
    sName.AppendPath("mesh3.ezMeshAsset");
    pDoc = static_cast<ezAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, ezDocumentFlags::RequestWindow));
    EZ_TEST_BOOL(pDoc != nullptr);
    sAbsAssetPath = pDoc->GetDocumentPath();
    documentGuid = pDoc->GetGuid();
    ProcessEvents();

    WaitForFileEvents(2, 1);
    ezFileStatus stat;
    stat.m_DocumentID = documentGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetPath), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sAbsAssetPath), stat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetPath, documentGuid, ezAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));

    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetPath);
  }

  ezStringBuilder sAbsAssetCopyPath = sAbsAssetPath;
  sAbsAssetCopyPath.ChangeFileName("meshCopy");
  ezUuid copyGuid;

  // Tests that copy gets a unique ID to resolev conflict.
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy Asset")
  {
    const ezUuid mod = ezUuid::MakeStableUuidFromString(sAbsAssetCopyPath);
    copyGuid = documentGuid;
    copyGuid.CombineWithSeed(mod);

    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log, true);
    log.ExpectMessage("Two assets have identical GUIDs:", ezLogMsgType::ErrorMsg, 1);

    ezOSFile::CopyFile(sAbsAssetPath, sAbsAssetCopyPath).AssertSuccess("Failed to copy file");

    WaitForFileEvents(3, 1);
    ezFileStatus stat;
    stat.m_DocumentID = copyGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, ezFileChangedEvent::Type::FileChanged),
      ezFileChangedEvent(MakePath(sAbsAssetCopyPath), stat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetCopyPath, copyGuid, ezAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));

    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetPath);
    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(copyGuid), copyGuid, sAbsAssetCopyPath);
  }

  ezStringBuilder sAbsAssetCopyRenamedPath = sAbsAssetCopyPath;
  sAbsAssetCopyRenamedPath.ChangeFileName("meshCopyRenamed");

  // Simple rename of not opened document
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rename Copied Asset")
  {
    ezOSFile::MoveFileOrDirectory(sAbsAssetCopyPath, sAbsAssetCopyRenamedPath).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    ezFileStatus stat;
    stat.m_DocumentID = copyGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sAbsAssetCopyPath), stat, ezFileChangedEvent::Type::DocumentUnlinked),
      ezFileChangedEvent(MakePath(sAbsAssetCopyPath), {}, ezFileChangedEvent::Type::FileRemoved),
      ezFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), stat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, ezAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, ezAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));

    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyPath));
    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(copyGuid), copyGuid, sAbsAssetCopyRenamedPath);
  }


  m_pApplication->m_pEditorApp->OpenDocument(sAbsAssetCopyRenamedPath, ezDocumentFlags::RequestWindow);
  ProcessEvents();

  // Delete an open document, make sure document gets closed.
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Copied Asset")
  {
    ezOSFile::DeleteFile(sAbsAssetCopyRenamedPath).AssertSuccess("Failed to delete file");

    WaitForFileEvents(1, 1);
    ezFileStatus stat;
    stat.m_DocumentID = copyGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetCopyRenamedPath), {}, ezFileChangedEvent::Type::FileRemoved)};
    AssetEvent expected2[] = {AssetEvent(sAbsAssetCopyRenamedPath, copyGuid, ezAssetCuratorEvent::Type::AssetRemoved)};
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));
    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyPath));
    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetCopyRenamedPath));
    EZ_TEST_BOOL(ezDocumentManager::GetDocumentByGuid(copyGuid) == nullptr);
  }

  ezStringBuilder sAbsAssetRenamedPath = sAbsAssetPath;
  sAbsAssetRenamedPath.ChangeFileName("meshRenamed");
  // Test that a renamed asset renames the open document as well.
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rename Asset")
  {
    ezOSFile::MoveFileOrDirectory(sAbsAssetPath, sAbsAssetRenamedPath).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    ezFileStatus stat;
    stat.m_DocumentID = documentGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sAbsAssetPath), stat, ezFileChangedEvent::Type::DocumentUnlinked),
      ezFileChangedEvent(MakePath(sAbsAssetPath), {}, ezFileChangedEvent::Type::FileRemoved),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath), stat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath, documentGuid, ezAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetRenamedPath, documentGuid, ezAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));
    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetPath));
    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetRenamedPath);

    EZ_TEST_STRING(pDoc->GetDocumentPath(), sAbsAssetRenamedPath);
  }

  ezStringBuilder sAbsAssetRenamedPath2 = sAbsAssetPath;
  sAbsAssetRenamedPath2.ChangeFileName("MeshRenameD");
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rename Asset Casing Only")
  {
    ezOSFile::MoveFileOrDirectory(sAbsAssetRenamedPath, sAbsAssetRenamedPath2).AssertSuccess("Failed to rename file");

    WaitForFileEvents(4, 2);
    ezFileStatus stat;
    stat.m_DocumentID = documentGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath2), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath), stat, ezFileChangedEvent::Type::DocumentUnlinked),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath), {}, ezFileChangedEvent::Type::FileRemoved),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, ezAssetCuratorEvent::Type::AssetMoved),    // Moved
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, ezAssetCuratorEvent::Type::AssetUpdated)}; // Asset transform state updated
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));
    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetPath));
    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid), documentGuid, sAbsAssetRenamedPath2);

    EZ_TEST_STRING(pDoc->GetDocumentPath(), sAbsAssetRenamedPath2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Overwrite asset with a different asset")
  {
    const ezDocumentTypeDescriptor* pTypeDesc = pDoc->GetAssetDocumentTypeDescriptor();

    ezUuid overwriteGuid = documentGuid;
    overwriteGuid.CombineWithSeed(ezUuid::MakeStableUuidFromString("Overwritten"));

    ezStringBuilder sTemp;
    ezStringBuilder sTempTarget = ezOSFile::GetTempDataFolder();
    sTempTarget.AppendPath(ezPathUtils::GetFileNameAndExtension(sAbsAssetRenamedPath2));
    sTempTarget.ChangeFileName(ezConversionUtils::ToString(overwriteGuid, sTemp));

    EZ_TEST_RESULT(pTypeDesc->m_pManager->CloneDocument(sAbsAssetRenamedPath2, sTempTarget, overwriteGuid).m_Result);
    EZ_TEST_RESULT(ezOSFile::CopyFile(sTempTarget, sAbsAssetRenamedPath2));
    ezOSFile::DeleteFile(sTempTarget).IgnoreResult();

    WaitForFileEvents(3, 2);

    ezFileStatus stat;
    stat.m_DocumentID = documentGuid;
    ezFileStatus newStat;
    stat.m_DocumentID = overwriteGuid;
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, ezFileChangedEvent::Type::FileChanged),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath2), stat, ezFileChangedEvent::Type::DocumentUnlinked),
      ezFileChangedEvent(MakePath(sAbsAssetRenamedPath2), newStat, ezFileChangedEvent::Type::DocumentLinked)};
    AssetEvent expected2[] = {
      AssetEvent(sAbsAssetRenamedPath2, documentGuid, ezAssetCuratorEvent::Type::AssetRemoved),
      AssetEvent(sAbsAssetRenamedPath2, overwriteGuid, ezAssetCuratorEvent::Type::AssetAdded)};
    CompareFiles(ezMakeArrayPtr(expected), ezMakeArrayPtr(expected2));
    EZ_TEST_BOOL(ezAssetCurator::GetSingleton()->FindSubAsset(sAbsAssetRenamedPath2));
    EZ_TEST_BOOL(!ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid));
    CheckSubAsset(ezAssetCurator::GetSingleton()->GetSubAsset(overwriteGuid), overwriteGuid, sAbsAssetRenamedPath2);
  }
}
