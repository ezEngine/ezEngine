#include <FoundationTest/FoundationTestPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/ThreadUtils.h>

namespace DirectoryWatcherTestHelpers
{

  struct ExpectedEvent
  {
    ~ExpectedEvent(){}; // NOLINT: To make it non-pod

    const char* path;
    ezDirectoryWatcherAction action;
    ezDirectoryWatcherType type;

    bool operator==(const ExpectedEvent& other) const
    {
      return ezStringView(path) == ezStringView(other.path) && action == other.action && type == other.type;
    }
  };

  struct ExpectedEventStorage
  {
    ezString path;
    ezDirectoryWatcherAction action;
    ezDirectoryWatcherType type;
  };

  void TickWatcher(ezDirectoryWatcher& ref_watcher)
  {
    ref_watcher.EnumerateChanges([&](ezStringView sPath, ezDirectoryWatcherAction action, ezDirectoryWatcherType type) {},
      ezTime::MakeFromMilliseconds(100));
  }
} // namespace DirectoryWatcherTestHelpers


void DirectoryWatcherTest()
{
  using namespace DirectoryWatcherTestHelpers;

  ezStringBuilder tmp, tmp2;
  ezStringBuilder sTestRootPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sTestRootPath.AppendPath("DirectoryWatcher/");

  auto CheckExpectedEvents = [&](ezDirectoryWatcher& ref_watcher, ezArrayPtr<ExpectedEvent> events)
  {
    ezDynamicArray<ExpectedEventStorage> firedEvents;
    ezUInt32 i = 0;
    ref_watcher.EnumerateChanges([&](ezStringView sPath, ezDirectoryWatcherAction action, ezDirectoryWatcherType type)
      {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      if (i < events.GetCount())
      {
        EZ_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
        EZ_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
        EZ_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
      }
      i++; },
      ezTime::MakeFromMilliseconds(100));
    EZ_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsUnordered = [&](ezDirectoryWatcher& ref_watcher, ezArrayPtr<ExpectedEvent> events)
  {
    ezDynamicArray<ExpectedEventStorage> firedEvents;
    ezUInt32 i = 0;
    ezDynamicArray<bool> eventFired;
    eventFired.SetCount(events.GetCount());
    ref_watcher.EnumerateChanges([&](ezStringView sPath, ezDirectoryWatcherAction action, ezDirectoryWatcherType type)
      {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        auto index = events.IndexOf({tmp, action, type});
        EZ_TEST_BOOL_MSG(index != ezInvalidIndex, "Event %d (%s, %d, %d) not found in expected events list", i, tmp.GetData(), (int)action, (int)type);
        if (index != ezInvalidIndex)
        {
          eventFired[index] = true;
        }
        i++;
        //
      },
      ezTime::MakeFromMilliseconds(100));
    for (auto& fired : eventFired)
    {
      EZ_TEST_BOOL(fired);
    }
    EZ_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsMultiple = [&](ezArrayPtr<ezDirectoryWatcher*> watchers, ezArrayPtr<ExpectedEvent> events)
  {
    ezDynamicArray<ExpectedEventStorage> firedEvents;
    ezUInt32 i = 0;
    ezDirectoryWatcher::EnumerateChanges(
      watchers, [&](ezStringView sPath, ezDirectoryWatcherAction action, ezDirectoryWatcherType type)
      {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        if (i < events.GetCount())
        {
          EZ_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
          EZ_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
          EZ_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
        }
        i++;
        //
      },
      ezTime::MakeFromMilliseconds(100));
    EZ_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CreateFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    ezOSFile file;
    EZ_TEST_BOOL(file.Open(tmp, ezFileOpenMode::Write).Succeeded());
    EZ_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto ModifyFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    ezOSFile file;
    EZ_TEST_BOOL(file.Open(tmp, ezFileOpenMode::Append).Succeeded());
    EZ_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto DeleteFile = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    EZ_TEST_BOOL(ezOSFile::DeleteFile(tmp).Succeeded());
  };

  auto CreateDirectory = [&](const char* szRelPath)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(tmp).Succeeded());
  };

  auto Rename = [&](const char* szFrom, const char* szTo)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szFrom);

    tmp2 = sTestRootPath;
    tmp2.AppendPath(szTo);

    EZ_TEST_BOOL(ezOSFile::MoveFileOrDirectory(tmp, tmp2).Succeeded());
  };

  auto DeleteDirectory = [&](const char* szRelPath, bool bTest = true)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    tmp.MakeCleanPath();

    if (bTest)
    {
      EZ_TEST_BOOL(ezOSFile::DeleteFolder(tmp).Succeeded());
    }
    else
    {
      ezOSFile::DeleteFolder(tmp).IgnoreResult();
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "git")
  {
    ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    CreateFile("index");

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("index.lock");
    DeleteFile("index");
    Rename("index.lock", "index");

    ExpectedEvent expectedEvents[] = {
      {"index.lock", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"index", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"index.lock", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::File},
      {"index", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::File},

    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple Create File")
  {
    ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple delete file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple modify file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    TickWatcher(watcher);

    ModifyFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("test.file");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple rename file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("test.file");
    Rename("test.file", "supertest.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"test.file", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::File},
      {"supertest.file", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("supertest.file");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Change file casing")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    Rename("rename.file", "Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"rename.file", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::File},
      {"Rename.file", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("Rename.file");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Windows check for correct handling of pending file remove event #1")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"rename.file", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Windows check for correct handling of pending file remove event #2")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");
    CreateFile("Rename.file");
    DeleteFile("Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"rename.file", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"Rename.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"Rename.file", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple create directory")
  {
    ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Creates).Succeeded());

    CreateDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple delete directory")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple rename directory")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames).Succeeded());

    CreateDirectory("testDir");
    Rename("testDir", "supertestDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::Directory},
      {"supertestDir", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("supertestDir");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Change directory casing")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    Rename("renameDir", "RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"renameDir", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::Directory},
      {"RenameDir", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("RenameDir");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Windows check for correct handling of pending directory remove event #1")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"renameDir", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Windows check for correct handling of pending directory remove event #2")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames | ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");
    CreateDirectory("RenameDir");
    DeleteDirectory("RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"renameDir", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
      {"RenameDir", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"RenameDir", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Subdirectory Create File")
  {
    tmp = sTestRootPath;
    tmp.AppendPath("subdir");
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(tmp).Succeeded());

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Subdirectory delete file")
  {

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    DeleteFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Subdirectory modify file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    TickWatcher(watcher);

    ModifyFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GUI Create Folder & file")
  {
    DeleteDirectory("sub", false);
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("New Folder", "sub");

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
      {"sub/bla", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GUI Create Folder & file fast")
  {
    DeleteDirectory("sub", false);
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");
    Rename("New Folder", "sub");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
      {"sub/bla", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GUI Create Folder & file fast subdir")
  {
    DeleteDirectory("sub", false);

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder/subsub");
    Rename("New Folder", "sub");

    TickWatcher(watcher);

    CreateFile("sub/subsub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/subsub/bla", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/subsub/bla");
    DeleteFile("sub/subsub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/subsub/bla", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
      {"sub/subsub/bla", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

    DeleteDirectory("sub");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GUI Delete Folder")
  {
    DeleteDirectory("sub2", false);
    DeleteDirectory("../sub2", false);

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/file1", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    Rename("sub2", "../sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
      {"sub2/file1", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"sub2", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
    };
    // Issue here: After moving sub2 out of view, it remains in m_pathToWd
    CheckExpectedEvents(watcher, expectedEvents2);

    Rename("../sub2", "sub2");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/file1", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create, Delete, Create")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/file1", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    DeleteDirectory("sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/file1", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
      {"sub2", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::Directory},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents2);

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/file1", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
      {"sub2/subsub2", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GUI Create file & delete")
  {
    DeleteDirectory("sub", false);
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                            ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("file2.txt", "datei2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"file2.txt", ezDirectoryWatcherAction::RenamedOldName, ezDirectoryWatcherType::File},
      {"datei2.txt", ezDirectoryWatcherAction::RenamedNewName, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    DeleteFile("datei2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"datei2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enumerate multiple")
  {
    DeleteDirectory("watch1", false);
    DeleteDirectory("watch2", false);
    DeleteDirectory("watch3", false);
    ezDirectoryWatcher watchers[3];

    ezDirectoryWatcher* pWatchers[] = {watchers + 0, watchers + 1, watchers + 2};

    CreateDirectory("watch1");
    CreateDirectory("watch2");
    CreateDirectory("watch3");

    ezStringBuilder watchPath;

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch1");
    EZ_TEST_BOOL(watchers[0].OpenDirectory(
                              watchPath,
                              ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                                ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch2");
    EZ_TEST_BOOL(watchers[1].OpenDirectory(
                              watchPath,
                              ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                                ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch3");
    EZ_TEST_BOOL(watchers[2].OpenDirectory(
                              watchPath,
                              ezDirectoryWatcher::Watch::Creates | ezDirectoryWatcher::Watch::Deletes |
                                ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("watch1/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"watch1/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents1);

    CreateFile("watch2/file2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"watch2/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents2);

    CreateFile("watch3/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"watch3/file2.txt", ezDirectoryWatcherAction::Added, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents3);

    ModifyFile("watch1/file2.txt");
    ModifyFile("watch2/file2.txt");

    ExpectedEvent expectedEvents4[] = {
      {"watch1/file2.txt", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
      {"watch2/file2.txt", ezDirectoryWatcherAction::Modified, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents4);

    DeleteFile("watch1/file2.txt");
    DeleteFile("watch2/file2.txt");
    DeleteFile("watch3/file2.txt");

    ExpectedEvent expectedEvents5[] = {
      {"watch1/file2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"watch2/file2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
      {"watch3/file2.txt", ezDirectoryWatcherAction::Removed, ezDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents5);
  }

  ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
}

EZ_CREATE_SIMPLE_TEST(IO, DirectoryWatcher)
{
  DirectoryWatcherTest();
}

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
EZ_CREATE_SIMPLE_TEST(IO, DirectoryWatcherNonNTFS)
{
  auto* pForceNonNTFS = static_cast<ezCVarBool*>(ezCVar::FindCVarByName("Platform.DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS = true;
  DirectoryWatcherTest();
  *pForceNonNTFS = false;
}
#  endif

#endif
