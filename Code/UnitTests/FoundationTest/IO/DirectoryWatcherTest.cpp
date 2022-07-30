#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/OSFile.h>

namespace DirectoryWatcherTestHelpers
{

  struct ExpectedEvent
  {
    EZ_DECLARE_POD_TYPE();

    const char* path;
    ezDirectoryWatcherAction action;
  };

  void TickWatcher(ezDirectoryWatcher& watcher)
  {
    ezUInt32 numEvents = 0;
    watcher.EnumerateChanges([&](const char* path, ezDirectoryWatcherAction action)
    {
      numEvents++;
    });
    EZ_TEST_BOOL(numEvents == 0);
  }
} // namespace DirectoryWatcherTestHelpers

EZ_CREATE_SIMPLE_TEST(IO, DirectoryWatcher)
{
  using namespace DirectoryWatcherTestHelpers;

  ezStringBuilder tmp, tmp2;
  ezStringBuilder sTestRootPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sTestRootPath.AppendPath("DirectoryWatcher/");

  auto CheckExpectedEvents = [&](ezDirectoryWatcher& watcher, ezArrayPtr<ExpectedEvent> events) {
    ezDynamicArray<ExpectedEvent> firedEvents;
    ezUInt32 i = 0;
    watcher.EnumerateChanges([&](const char* path, ezDirectoryWatcherAction action) {
      tmp = path;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action});
      if (i < events.GetCount())
      {
        EZ_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
        EZ_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
      }
      i++;
    });
    EZ_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CreateFile = [&](const char* relPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(relPath);

    ezOSFile file;
    EZ_TEST_BOOL(file.Open(tmp, ezFileOpenMode::Write).Succeeded());
    EZ_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto ModifyFile = [&](const char* relPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(relPath);

    ezOSFile file;
    EZ_TEST_BOOL(file.Open(tmp, ezFileOpenMode::Append).Succeeded());
    EZ_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto DeleteFile = [&](const char* relPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(relPath);
    EZ_TEST_BOOL(ezOSFile::DeleteFile(tmp).Succeeded());
  };

  auto CreateDirectory = [&](const char* relPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(relPath);
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(tmp).Succeeded());
  };

  auto Rename = [&](const char* from, const char* to) {
    tmp = sTestRootPath;
    tmp.AppendPath(from);

    tmp2 = sTestRootPath;
    tmp2.AppendPath(to);

    EZ_TEST_BOOL(ezOSFile::MoveFileOrDirectory(tmp, tmp2).Succeeded());
  };

  auto DeleteDirectory = [&](const char* relPath, bool test = true)
  {
    tmp = sTestRootPath;
    tmp.AppendPath(relPath);

    if(test)
    {
      EZ_TEST_BOOL(ezOSFile::DeleteFolder(tmp).Succeeded());
    }
    else
    {
      ezOSFile::DeleteFolder(tmp).IgnoreResult();
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple Create File")
  {
    ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Creates).Succeeded());

    CreateFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple delete file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple modify file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");
    ModifyFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::Modified},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("test.file");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple rename file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Renames).Succeeded());

    CreateFile("test.file");
    Rename("test.file", "supertest.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", ezDirectoryWatcherAction::RenamedOldName},
      {"supertest.file", ezDirectoryWatcherAction::RenamedNewName},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("supertest.file");
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
      {"subdir/test.file", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Subdirectory delete file")
  {

    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Deletes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    DeleteFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Subdirectory modify file")
  {
    ezDirectoryWatcher watcher;
    EZ_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, ezDirectoryWatcher::Watch::Writes | ezDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");
    ModifyFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", ezDirectoryWatcherAction::Modified},
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

    TickWatcher(watcher);

    Rename("New Folder", "sub");

    CreateFile("sub/bla");
    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents[] = {
      {"sub/bla", ezDirectoryWatcherAction::Added},
      {"sub/bla", ezDirectoryWatcherAction::Modified},
      {"sub/bla", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents);
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

    TickWatcher(watcher);

    CreateFile("sub/bla");
    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents[] = {
      {"sub/bla", ezDirectoryWatcherAction::Added},
      {"sub/bla", ezDirectoryWatcherAction::Modified},
      {"sub/bla", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents);
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
    ModifyFile("sub/subsub/bla");
    DeleteFile("sub/subsub/bla");

    ExpectedEvent expectedEvents[] = {
      {"sub/subsub/bla", ezDirectoryWatcherAction::Added},
      {"sub/subsub/bla", ezDirectoryWatcherAction::Modified},
      {"sub/subsub/bla", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents);

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
      {"sub2/file1", ezDirectoryWatcherAction::Added},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("sub2", "../sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Removed},
      {"sub2/file1", ezDirectoryWatcherAction::Removed},
    };
    // Issue here: After moving sub2 out of view, it remains in m_pathToWd
    CheckExpectedEvents(watcher, expectedEvents2);

    Rename("../sub2", "sub2");

    ExpectedEvent expectedEvents3[] = {
      {"sub2/file1", ezDirectoryWatcherAction::Added},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

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
      {"sub2/file1", ezDirectoryWatcherAction::Added},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    DeleteDirectory("sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/file1", ezDirectoryWatcherAction::Removed},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Removed},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"sub2/file1", ezDirectoryWatcherAction::Added},
      {"sub2/subsub2/file2.txt", ezDirectoryWatcherAction::Added},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  ezOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
}