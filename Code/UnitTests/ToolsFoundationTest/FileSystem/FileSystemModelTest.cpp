#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER) && EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Threading/ThreadUtils.h>
#  include <ToolsFoundation/FileSystem/FileSystemModel.h>


EZ_CREATE_SIMPLE_TEST_GROUP(FileSystem);

namespace
{
  ezResult eztCreateFile(ezStringView sPath)
  {
    ezFileWriter FileOut;
    EZ_SUCCEED_OR_RETURN(FileOut.Open(sPath));
    EZ_SUCCEED_OR_RETURN(FileOut.WriteString("Test"));
    FileOut.Close();
    return EZ_SUCCESS;
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(FileSystem, DataDirPath)
{
  const ezStringView sFilePathView = "C:/Code/ezEngine/Data/Samples/Testing Chambers/Objects/Barrel.ezPrefab"_ezsv;
  const ezStringView sDataDirView = "C:/Code/ezEngine/Data/Samples/Testing Chambers"_ezsv;

  auto CheckIsValid = [&](const ezDataDirPath& path)
  {
    EZ_TEST_BOOL(path.IsValid());
    ezStringView sAbs = path.GetAbsolutePath();
    EZ_TEST_STRING(sAbs, sFilePathView);
    ezStringView sDD = path.GetDataDir();
    EZ_TEST_STRING(sDD, sDataDirView);
    ezStringView sPR = path.GetDataDirParentRelativePath();
    EZ_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.ezPrefab");
    ezStringView sR = path.GetDataDirRelativePath();
    EZ_TEST_STRING(sR, "Objects/Barrel.ezPrefab");
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Windows Path copy ctor")
  {
    ezHybridArray<ezString, 2> rootFolders;
    rootFolders.PushBack("C:/SomeOtherFolder/Folder");
    rootFolders.PushBack(sDataDirView);

    ezDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    ezUInt32 uiIndex = path.GetDataDirIndex();
    EZ_TEST_INT(uiIndex, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Linux Path move ctor")
  {
    ezString sFilePathView = "/Code/ezEngine/Data/Samples/Testing Chambers/Objects/Barrel.ezPrefab"_ezsv;
    ezString sFilePath = sFilePathView;
    auto sDataDir = "/Code/ezEngine/Data/Samples/Testing Chambers"_ezsv;
    ezHybridArray<ezString, 2> rootFolders;
    rootFolders.PushBack(sDataDir);
    rootFolders.PushBack("/SomeOtherFolder/Folder");

    const char* szRawStringPtr = sFilePath.GetData();
    ezDataDirPath path(std::move(sFilePath), rootFolders);
    EZ_TEST_BOOL(path.IsValid());
    ezStringView sAbs = path.GetAbsolutePath();
    EZ_TEST_STRING(sAbs, sFilePathView);
    EZ_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());
    ezStringView sDD = path.GetDataDir();
    EZ_TEST_STRING(sDD, sDataDir);
    ezStringView sPR = path.GetDataDirParentRelativePath();
    EZ_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.ezPrefab");
    ezStringView sR = path.GetDataDirRelativePath();
    EZ_TEST_STRING(sR, "Objects/Barrel.ezPrefab");
    ezUInt32 uiIndex = path.GetDataDirIndex();
    EZ_TEST_INT(uiIndex, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Path to DataDir Itself")
  {
    ezString sDataDirView = (const char*)u8"/Code/ezEngine/Data/Sämples/Testing Chämbers";
    ezHybridArray<ezString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    ezDataDirPath path(sDataDirView.GetView(), rootFolders);
    EZ_TEST_BOOL(path.IsValid());
    ezStringView sAbs = path.GetAbsolutePath();
    EZ_TEST_STRING(sAbs, sDataDirView);
    ezStringView sDD = path.GetDataDir();
    EZ_TEST_STRING(sDD, sDataDirView);
    ezStringView sPR = path.GetDataDirParentRelativePath();
    EZ_TEST_STRING(sPR, (const char*)u8"Testing Chämbers");
    ezStringView sR = path.GetDataDirRelativePath();
    EZ_TEST_STRING(sR, "");
    ezUInt32 uiIndex = path.GetDataDirIndex();
    EZ_TEST_INT(uiIndex, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move")
  {
    ezHybridArray<ezString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    ezString sFilePath = sFilePathView;
    const char* szRawStringPtr = sFilePath.GetData();
    ezDataDirPath path(std::move(sFilePath), rootFolders);
    CheckIsValid(path);

    ezStringView sAbs = path.GetAbsolutePath();
    EZ_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());

    ezDataDirPath path2 = std::move(path);
    ezStringView sAbs2 = path2.GetAbsolutePath();
    EZ_TEST_BOOL(szRawStringPtr == sAbs2.GetStartPointer());

    ezDataDirPath path3(std::move(path2));
    ezStringView sAbs3 = path3.GetAbsolutePath();
    EZ_TEST_BOOL(szRawStringPtr == sAbs3.GetStartPointer());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rebuild")
  {
    ezHybridArray<ezString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    ezDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    EZ_TEST_INT(path.GetDataDirIndex(), 0);

    ezHybridArray<ezString, 2> newRootFolders;
    newRootFolders.PushBack(sDataDirView);
    newRootFolders.PushBack("C:/Some/Other/DataDir");

    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    EZ_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.InsertAt(0, "C:/Some/Other/DataDir2");
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    EZ_TEST_INT(path.GetDataDirIndex(), 1);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    EZ_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    EZ_TEST_BOOL(!path.IsValid());
    ezStringView sAbs = path.GetAbsolutePath();
    EZ_TEST_STRING(sAbs, sFilePathView);
  }
}

void FileSystemModelTest()
{
  constexpr ezUInt32 WAIT_LOOPS = 1000;

  ezStringBuilder sOutputFolder = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("Model");
  sOutputFolder.MakeCleanPath();

  ezStringBuilder sOutputFolderResolved;
  ezFileSystem::ResolveSpecialDirectory(sOutputFolder, sOutputFolderResolved).IgnoreResult();

  ezHybridArray<ezString, 1> rootFolders;

  ezApplicationFileSystemConfig fsConfig;
  ezApplicationFileSystemConfig::DataDirConfig& dataDir = fsConfig.m_DataDirs.ExpandAndGetRef();
  dataDir.m_bWritable = true;
  dataDir.m_sDataDirSpecialPath = sOutputFolder;
  dataDir.m_sRootName = "output";
  rootFolders.PushBack(sOutputFolder);

  // Files
  ezHybridArray<ezFileChangedEvent, 2> fileEvents;
  ezHybridArray<ezTime, 2> fileEventTimestamps;
  ezMutex fileEventLock;
  auto fileEvent = [&](const ezFileChangedEvent& e)
  {
    EZ_LOCK(fileEventLock);
    fileEvents.PushBack(e);
    fileEventTimestamps.PushBack(ezTime::Now());

    ezFileStatus stat;
    switch (e.m_Type)
    {
      case ezFileChangedEvent::Type::FileRemoved:
        EZ_TEST_BOOL(ezFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Failed());
        break;
      case ezFileChangedEvent::Type::FileAdded:
      case ezFileChangedEvent::Type::FileChanged:
      case ezFileChangedEvent::Type::DocumentLinked:
        EZ_TEST_BOOL(ezFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Succeeded());
        break;

      case ezFileChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  ezEventSubscriptionID fileId = ezFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(fileEvent);

  // Folders
  ezHybridArray<ezFolderChangedEvent, 2> folderEvents;
  ezHybridArray<ezTime, 2> folderEventTimestamps;
  ezMutex folderEventLock;
  auto folderEvent = [&](const ezFolderChangedEvent& e)
  {
    EZ_LOCK(folderEventLock);
    folderEvents.PushBack(e);
    folderEventTimestamps.PushBack(ezTime::Now());

    switch (e.m_Type)
    {
      case ezFolderChangedEvent::Type::FolderAdded:
        EZ_TEST_BOOL(ezFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
        break;
      case ezFolderChangedEvent::Type::FolderRemoved:
        EZ_TEST_BOOL(!ezFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
        break;
      case ezFolderChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  ezEventSubscriptionID folderId = ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(folderEvent);

  // Helper functions
  auto CompareFiles = [&](ezArrayPtr<ezFileChangedEvent> expected)
  {
    EZ_LOCK(fileEventLock);
    if (EZ_TEST_INT(expected.GetCount(), fileEvents.GetCount()))
    {
      for (ezUInt32 i = 0; i < expected.GetCount(); i++)
      {
        EZ_TEST_INT((int)expected[i].m_Type, (int)fileEvents[i].m_Type);
        EZ_TEST_STRING(expected[i].m_Path, fileEvents[i].m_Path);
        EZ_TEST_BOOL(expected[i].m_Status.m_DocumentID == fileEvents[i].m_Status.m_DocumentID);
        // Ignore stats besudes GUID.
      }
    }
  };

  auto ClearFiles = [&]()
  {
    EZ_LOCK(fileEventLock);
    fileEvents.Clear();
    fileEventTimestamps.Clear();
  };

  auto CompareFolders = [&](ezArrayPtr<ezFolderChangedEvent> expected)
  {
    EZ_LOCK(folderEventLock);
    if (EZ_TEST_INT(expected.GetCount(), folderEvents.GetCount()))
    {
      for (ezUInt32 i = 0; i < expected.GetCount(); i++)
      {
        EZ_TEST_INT((int)expected[i].m_Type, (int)folderEvents[i].m_Type);
        EZ_TEST_STRING(expected[i].m_Path, folderEvents[i].m_Path);
        // Ignore stats
      }
    }
  };

  auto ClearFolders = [&]()
  {
    EZ_LOCK(folderEventLock);
    folderEvents.Clear();
    folderEventTimestamps.Clear();
  };

  auto MakePath = [&](ezStringView sPath)
  {
    return ezDataDirPath(sPath, rootFolders);
  };


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Startup")
  {
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    EZ_TEST_RESULT(ezOSFile::DeleteFolder(sOutputFolderResolved));
    EZ_TEST_RESULT(ezFileSystem::CreateDirectoryStructure(sOutputFolderResolved));

    // for absolute paths
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory("", "", ":", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);

    ezFileSystemModel::GetSingleton()->Initialize(fsConfig, {}, {});

    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    auto it = ezFileSystemModel::GetSingleton()->GetFolders()->GetIterator();
    EZ_TEST_STRING(it.Key(), sOutputFolder);
    EZ_TEST_BOOL(it.Value() == ezFileStatus::Status::Valid);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "git")
  {
    ezStringBuilder sIndex(sOutputFolder);
    sIndex.AppendPath("index");
    ezStringBuilder sLock(sOutputFolder);
    sLock.AppendPath("index.lock");

    EZ_TEST_RESULT(eztCreateFile(sIndex));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sIndex), {}, ezFileChangedEvent::Type::FileAdded)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
    }

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
    // As we intend to swap the index and index.lock files later, we need to make sure the two files have sufficiently different modification dates so that the swap of the files is detected as a change to the original file.
    ezThreadUtils::Sleep(ezTime::MakeFromSeconds(1.0));
#  endif

    EZ_TEST_RESULT(eztCreateFile(sLock));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sLock), {}, ezFileChangedEvent::Type::FileAdded)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
    }

    EZ_TEST_RESULT(ezOSFile::DeleteFile(sIndex));
    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sLock, sIndex));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() >= 2)
        break;
    }

    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sIndex), {}, ezFileChangedEvent::Type::FileChanged),
      ezFileChangedEvent(MakePath(sLock), {}, ezFileChangedEvent::Type::FileRemoved)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    // Cleanup test
    EZ_TEST_RESULT(ezOSFile::DeleteFile(sIndex));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    ClearFiles();
    ClearFolders();
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Add file")
  {
    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    EZ_TEST_RESULT(eztCreateFile(sFilePath));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileAdded)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "modify file")
  {
    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    {
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
      // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
      ezThreadUtils::Sleep(ezTime::MakeFromSeconds(1.0));
#  endif
      ezFileWriter FileOut;
      EZ_TEST_RESULT(FileOut.Open(sFilePath));
      EZ_TEST_RESULT(FileOut.WriteString("Test2"));
      EZ_TEST_RESULT(FileOut.Flush());
      FileOut.Close();
    }

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileChanged)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "rename file")
  {
    ezStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile.txt");

    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("rootFile2.txt");

    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sFilePathNew), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sFilePathOld), {}, ezFileChangedEvent::Type::FileRemoved)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Add folder")
  {
    ezStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder1");

    EZ_TEST_RESULT(ezFileSystem::CreateDirectoryStructure(sFolderPath));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(folderEventLock);
      if (folderEvents.GetCount() > 0)
        break;
    }

    ezFolderChangedEvent expected[] = {ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderAdded)};
    CompareFolders(ezMakeArrayPtr(expected));
    ClearFolders();
    CompareFiles({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "move file")
  {
    ezStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile2.txt");

    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder1", "rootFile2.txt");

    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sFilePathNew), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sFilePathOld), {}, ezFileChangedEvent::Type::FileRemoved)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "move folder")
  {
    ezStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder1");

    ezStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder1", "rootFile2.txt");

    ezStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("Folder12");

    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      EZ_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      ezFolderChangedEvent expected[] = {
        ezFolderChangedEvent(MakePath(sFolderPathNew), ezFolderChangedEvent::Type::FolderAdded),
        ezFolderChangedEvent(MakePath(sFolderPathOld), ezFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(ezMakeArrayPtr(expected));
    }

    {
      ezFileChangedEvent expected[] = {
        ezFileChangedEvent(MakePath(sFilePathNew), {}, ezFileChangedEvent::Type::FileAdded),
        ezFileChangedEvent(MakePath(sFilePathOld), {}, ezFileChangedEvent::Type::FileRemoved)};
      CompareFiles(ezMakeArrayPtr(expected));
    }
    {
      EZ_LOCK(fileEventLock);
      EZ_LOCK(folderEventLock);
      // Check folder added before file
      EZ_TEST_BOOL(fileEventTimestamps[0] > folderEventTimestamps[0]);
      // Check file removed before folder
      EZ_TEST_BOOL(fileEventTimestamps[1] < folderEventTimestamps[1]);
    }

    ClearFolders();
    ClearFiles();

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HashFile")
  {
    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    ezFileStatus status;
    EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->HashFile(sFilePathNew, status));
    EZ_TEST_INT((ezInt64)status.m_uiHash, (ezInt64)10983861097202158394u);
  }

  ezFileSystemModel::FilesMap referencedFiles;
  ezFileSystemModel::FoldersMap referencedFolders;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shutdown")
  {
    ezFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    EZ_TEST_INT(referencedFiles.GetCount(), 1);
    EZ_TEST_INT(referencedFolders.GetCount(), 2);

    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();
  }

  ezStringBuilder sOutputFolder2 = sOutputFolderResolved;
  sOutputFolder2.ChangeFileNameAndExtension("Model2");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Startup Restore Model")
  {
    {
      // Add another data directory. This is now at the index of the old one, requiring the indices to be updated inside ezFileSystemModel::Initialize.
      EZ_TEST_RESULT(ezOSFile::DeleteFolder(sOutputFolder2));
      EZ_TEST_RESULT(ezFileSystem::CreateDirectoryStructure(sOutputFolder2));
      ezApplicationFileSystemConfig::DataDirConfig dataDir;
      dataDir.m_bWritable = true;
      dataDir.m_sDataDirSpecialPath = sOutputFolder2;
      dataDir.m_sRootName = "output2";

      rootFolders.InsertAt(0, sOutputFolder);
      fsConfig.m_DataDirs.InsertAt(0, dataDir);
    }

    ezFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));

    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);

    // Check that files have be remapped.
    for (auto it : *ezFileSystemModel::GetSingleton()->GetFiles())
    {
      EZ_TEST_INT(it.Key().GetDataDirIndex(), 1);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFiles")
  {
    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    ezFileSystemModel::LockedFiles files = ezFileSystemModel::GetSingleton()->GetFiles();
    EZ_TEST_INT(files->GetCount(), 1);
    auto it = files->GetIterator();
    EZ_TEST_STRING(it.Key(), sFilePathNew);
    EZ_TEST_BOOL(it.Value().m_LastModified.IsValid());
    EZ_TEST_INT((ezInt64)it.Value().m_uiHash, (ezInt64)10983861097202158394u);
    EZ_TEST_BOOL(it.Value().m_Status == ezFileStatus::Status::Valid);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFolders")
  {
    ezStringBuilder sFolder(sOutputFolder);
    sFolder.AppendPath("Folder12");

    ezFileSystemModel::LockedFolders folders = ezFileSystemModel::GetSingleton()->GetFolders();
    EZ_TEST_INT(folders->GetCount(), 3);
    auto it = folders->GetIterator();

    // ezMap is sorted so the order is fixed.
    EZ_TEST_STRING(it.Key(), sOutputFolder);
    EZ_TEST_BOOL(it.Value() == ezFileStatus::Status::Valid);

    it.Next();
    EZ_TEST_STRING(it.Key(), sFolder);
    EZ_TEST_BOOL(it.Value() == ezFileStatus::Status::Valid);

    it.Next();
    EZ_TEST_STRING(it.Key(), sOutputFolder2);
    EZ_TEST_BOOL(it.Value() == ezFileStatus::Status::Valid);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CheckFileSystem")
  {
    ezStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder12");

    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "rootFile2.txt");

    ezFileSystemModel::GetSingleton()->CheckFileSystem();

    // #TODO_ASSET This FileChanged should be removed once the model is fixed to no longer require firing this after restoring the model from cache. See comment in ezFileSystemModel::HandleSingleFile.
    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileChanged),
      ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NotifyOfChange - File")
  {
    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");
    {
      EZ_TEST_RESULT(eztCreateFile(sFilePath));
      ezFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileAdded)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      EZ_TEST_RESULT(ezOSFile::DeleteFile(sFilePath));
      ezFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileRemoved)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    for (size_t i = 0; i < 15; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NotifyOfChange - Folder")
  {
    ezStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("AnotherFolder");
    {
      EZ_TEST_RESULT(ezFileSystem::CreateDirectoryStructure(sFolderPath));
      ezFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      ezFolderChangedEvent expected[] = {ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(ezMakeArrayPtr(expected));
      ClearFolders();
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 4);
    }

    {
      EZ_TEST_RESULT(ezOSFile::DeleteFolder(sFolderPath));
      ezFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      ezFolderChangedEvent expected[] = {ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(ezMakeArrayPtr(expected));
      ClearFolders();
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CheckFolder - File")
  {
    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "subFile.txt");
    {
      EZ_TEST_RESULT(eztCreateFile(sFilePath));
      ezFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileAdded)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      EZ_TEST_RESULT(ezOSFile::DeleteFile(sFilePath));
      ezFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileRemoved)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CheckFolder - Folder")
  {
    ezStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("YetAnotherFolder");
    ezStringBuilder sFolderSubPath(sOutputFolder);
    sFolderSubPath.AppendPath("YetAnotherFolder", "SubFolder");
    {
      EZ_TEST_RESULT(ezFileSystem::CreateDirectoryStructure(sFolderSubPath));
      ezFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      ezFolderChangedEvent expected[] = {
        ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderAdded),
        ezFolderChangedEvent(MakePath(sFolderSubPath), ezFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(ezMakeArrayPtr(expected));
      ClearFolders();
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 5);
    }

    {
      EZ_TEST_RESULT(ezOSFile::DeleteFolder(sFolderPath));
      ezFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      ezFolderChangedEvent expected[] = {
        ezFolderChangedEvent(MakePath(sFolderSubPath), ezFolderChangedEvent::Type::FolderRemoved),
        ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(ezMakeArrayPtr(expected));
      ClearFolders();
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadDocument")
  {
    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    ezUuid docGuid = ezUuid::MakeUuid();
    auto callback = [&](const ezFileStatus& status, ezStreamReader& ref_reader)
    {
      EZ_TEST_INT((ezInt64)status.m_uiHash, (ezInt64)10983861097202158394u);
      ezFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, docGuid).IgnoreResult();
    };

    EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->ReadDocument(sFilePathNew, callback));

    ezFileStatus stat;
    stat.m_DocumentID = docGuid;
    ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePathNew), stat, ezFileChangedEvent::Type::DocumentLinked)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LinkDocument")
  {
    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    ezUuid guid = ezUuid::MakeUuid();
    ezUuid guid2 = ezUuid::MakeUuid();
    {
      EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid2));

      ezFileStatus stat;
      stat.m_DocumentID = guid;
      ezFileStatus stat2;
      stat2.m_DocumentID = guid2;

      ezFileChangedEvent expected[] = {
        ezFileChangedEvent(MakePath(sFilePathNew), stat, ezFileChangedEvent::Type::DocumentLinked),
        ezFileChangedEvent(MakePath(sFilePathNew), stat, ezFileChangedEvent::Type::DocumentUnlinked),
        ezFileChangedEvent(MakePath(sFilePathNew), stat2, ezFileChangedEvent::Type::DocumentLinked)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
    }
    {
      EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));
      EZ_TEST_RESULT(ezFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));

      ezFileStatus stat2;
      stat2.m_DocumentID = guid2;

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePathNew), stat2, ezFileChangedEvent::Type::DocumentUnlinked)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
    }
    for (size_t i = 0; i < 15; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Change file casing")
  {
    ezStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder12", "rootFile2.txt");

    ezStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "RootFile2.txt");

    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    ezFileChangedEvent expected[] = {
      ezFileChangedEvent(MakePath(sFilePathNew), {}, ezFileChangedEvent::Type::FileAdded),
      ezFileChangedEvent(MakePath(sFilePathOld), {}, ezFileChangedEvent::Type::FileRemoved)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Change folder casing")
  {
    ezStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder12");

    ezStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("FOLDER12");

    EZ_TEST_RESULT(ezOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      ezFolderChangedEvent expected[] = {
        ezFolderChangedEvent(MakePath(sFolderPathNew), ezFolderChangedEvent::Type::FolderAdded),
        ezFolderChangedEvent(MakePath(sFolderPathOld), ezFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(ezMakeArrayPtr(expected));
      ClearFolders();
    }

    {
      ezStringBuilder sFilePathOld(sOutputFolder);
      sFilePathOld.AppendPath("Folder12", "RootFile2.txt");
      ezStringBuilder sFilePathNew(sOutputFolder);
      sFilePathNew.AppendPath("FOLDER12", "RootFile2.txt");

      ezFileChangedEvent expected[] = {
        ezFileChangedEvent(MakePath(sFilePathNew), {}, ezFileChangedEvent::Type::FileAdded),
        ezFileChangedEvent(MakePath(sFilePathOld), {}, ezFileChangedEvent::Type::FileRemoved)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
    }

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "delete folder")
  {
    ezStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("FOLDER12");

    ezStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("FOLDER12", "RootFile2.txt");

    EZ_TEST_RESULT(ezOSFile::DeleteFolder(sFolderPath));

    for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      ezFileSystemModel::GetSingleton()->MainThreadTick();
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

      EZ_LOCK(fileEventLock);
      EZ_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 1 && folderEvents.GetCount() == 1)
        break;
    }

    {
      ezFolderChangedEvent expected[] = {
        ezFolderChangedEvent(MakePath(sFolderPath), ezFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(ezMakeArrayPtr(expected));
    }

    {
      ezFileChangedEvent expected[] = {
        ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileRemoved)};
      CompareFiles(ezMakeArrayPtr(expected));
    }

    {
      EZ_LOCK(fileEventLock);
      EZ_LOCK(folderEventLock);
      // Check file removed before folder.
      EZ_TEST_BOOL(fileEventTimestamps[0] < folderEventTimestamps[0]);
    }

    ClearFolders();
    ClearFiles();

    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  referencedFiles = {};
  referencedFolders = {};

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shutdown with cached files and folders")
  {
    {
      // Add a file to test data directories being removed.
      ezStringBuilder sFilePath(sOutputFolder);
      sFilePath.AppendPath("rootFile.txt");

      EZ_TEST_RESULT(eztCreateFile(sFilePath));

      for (ezUInt32 i = 0; i < WAIT_LOOPS; i++)
      {
        ezFileSystemModel::GetSingleton()->MainThreadTick();
        ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));

        EZ_LOCK(fileEventLock);
        if (fileEvents.GetCount() > 0)
          break;
      }

      ezFileChangedEvent expected[] = {ezFileChangedEvent(MakePath(sFilePath), {}, ezFileChangedEvent::Type::FileAdded)};
      CompareFiles(ezMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
    }

    ezFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    EZ_TEST_INT(referencedFiles.GetCount(), 1);
    EZ_TEST_INT(referencedFolders.GetCount(), 2);

    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Startup without data dirs")
  {
    fsConfig.m_DataDirs.Clear();

    ezFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    EZ_TEST_INT(ezFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 0);

    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Final shutdown")
  {
    ezFileSystemModel::GetSingleton()->Deinitialize();
    ezFileChangedEvent expected[] = {ezFileChangedEvent({}, {}, ezFileChangedEvent::Type::ModelReset)};
    CompareFiles(ezMakeArrayPtr(expected));
    ClearFiles();

    ezFolderChangedEvent expected2[] = {ezFolderChangedEvent({}, ezFolderChangedEvent::Type::ModelReset)};
    CompareFolders(ezMakeArrayPtr(expected2));
    ClearFolders();

    ezFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(fileId);
    ezFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(folderId);
  }
}

EZ_CREATE_SIMPLE_TEST(FileSystem, FileSystemModel)
{
  FileSystemModelTest();
}

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
EZ_CREATE_SIMPLE_TEST(FileSystem, FileSystemModelNonNTFS)
{
  auto* pForceNonNTFS = static_cast<ezCVarBool*>(ezCVar::FindCVarByName("Platform.DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS = true;
  FileSystemModelTest();
  *pForceNonNTFS = false;
}
#  endif

#endif
