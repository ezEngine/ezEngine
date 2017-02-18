#include <PCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

EZ_CREATE_SIMPLE_TEST(IO, FileSystem)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  return;
#endif

  ezStringBuilder sFileContent = "Lyrics to Taste The Cake:\n\
Turret: Who's there?\n\
Turret: Is anyone there?\n\
Turret: I see you.\n\
\n\
Chell rises from a stasis inside of a glass box\n\
She isn't greeted by faces,\n\
Only concrete and clocks.\n\
...";

  const char* szOutputFolder = ">sdk/Output/Bin";
  ezStringBuilder sOutputFolderResolved;
  ezFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved);

  // Don't use 'BUILDSYSTEM_OUTPUT_FOLDER' in any other tests, it's just used here because we are too poor to get
  // an absolute path that actually exists from anywhere else.
  ezStringBuilder sOutputFolder1 = szOutputFolder;
  sOutputFolder1.AppendPath("FoundationTest", "IO", "SubFolder");
  ezStringBuilder sOutputFolder1Resolved;
  ezFileSystem::ResolveSpecialDirectory(sOutputFolder1, sOutputFolder1Resolved);

  ezStringBuilder sOutputFolder2 = szOutputFolder;
  sOutputFolder2.AppendPath("FoundationTest", "IO", "SubFolder2");
  ezStringBuilder sOutputFolder2Resolved;
  ezFileSystem::ResolveSpecialDirectory(sOutputFolder2, sOutputFolder2Resolved);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    //ezFileSystem::ClearAllDataDirectoryFactories();

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // for absolute paths
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(szOutputFolder, "Clear", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

    ezStringBuilder sTempFile = sOutputFolder1Resolved;
    sTempFile.AppendPath("Temp.tmp");

    ezFileWriter TempFile;
    EZ_TEST_BOOL(TempFile.Open(sTempFile) == EZ_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2Resolved;
    sTempFile.AppendPath("Temp.tmp");

    EZ_TEST_BOOL(TempFile.Open(sTempFile) == EZ_SUCCESS);
    TempFile.Close();

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1, "Clear", "output1", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Clear") == EZ_SUCCESS);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == EZ_SUCCESS);

    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 3);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == EZ_SUCCESS);

    ezFileSystem::ClearAllDataDirectories();

    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 0);
    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Clear") == 0);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1, "", "output1", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write File")
  {
    ezFileWriter FileOut;

    ezStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileOut.Open(":output1/FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST_BOOL(FileOut.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileOut.GetFilePathAbsolute() == sAbs);

    EZ_TEST_BOOL(FileOut.GetFileSize() == 0);

    EZ_TEST_BOOL(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == EZ_SUCCESS);

    FileOut.Flush();
    EZ_TEST_BOOL(FileOut.GetFileSize() == sFileContent.GetElementCount());

    FileOut.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read File")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST_BOOL(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileIn.GetFilePathAbsolute() == sAbs);

    EZ_TEST_BOOL(FileIn.GetFileSize() == sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    EZ_TEST_BOOL(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read File (Absolute Path)")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileIn.Open(sAbs) == EZ_SUCCESS);

    EZ_TEST_BOOL(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileIn.GetFilePathAbsolute() == sAbs);

    EZ_TEST_BOOL(FileIn.GetFileSize() == sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    EZ_TEST_BOOL(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#endif

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete File / Exists File")
  {
    EZ_TEST_BOOL(ezFileSystem::ExistsFile(":output1/FileSystemTest.txt"));
    ezFileSystem::DeleteFile(":output1/FileSystemTest.txt");
    EZ_TEST_BOOL(!ezFileSystem::ExistsFile("FileSystemTest.txt"));

    ezFileReader FileIn;
    EZ_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileStats")
  {
    // Create file
    {
      ezFileWriter FileOut;
      ezStringBuilder sAbs = sOutputFolder1Resolved;
      sAbs.AppendPath("FileSystemTest.txt");
      EZ_TEST_BOOL(FileOut.Open(":output1/FileSystemTest.txt") == EZ_SUCCESS);
      FileOut.WriteBytes("Test", 4);
    }

    ezFileStats stat;

    EZ_TEST_BOOL(ezFileSystem::GetFileStats(":output1/FileSystemTest.txt", stat).Succeeded());

    EZ_TEST_BOOL(!stat.m_bIsDirectory);
    EZ_TEST_STRING(stat.m_sFileName, "FileSystemTest.txt");
    EZ_TEST_INT(stat.m_uiFileSize, 4);

    ezFileSystem::DeleteFile(":output1/FileSystemTest.txt");
    EZ_TEST_BOOL(ezFileSystem::GetFileStats(":output1/FileSystemTest.txt", stat).Failed());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ResolvePath")
  {
    ezStringBuilder sRel, sAbs;

    EZ_TEST_BOOL(ezFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == EZ_SUCCESS);

    ezStringBuilder sExpectedAbs = sOutputFolder1Resolved;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    EZ_TEST_BOOL(sAbs == sExpectedAbs);
    EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");

    // create a file in the second dir
    {
      EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", ezFileSystem::AllowWrites) == EZ_SUCCESS);

      {
        ezFileWriter FileOut;
        EZ_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == EZ_SUCCESS);
      }

      EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 1);
    }

    // find the path to an existing file
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath("FileSystemTest2.txt", &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder2Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath(":output1/SubSub/FileSystemTest2.txt", &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "SubSub/FileSystemTest2.txt");
    }

    ezFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    ezFileSystem::DeleteFile(":output2/FileSystemTest2.txt");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindFolderWithSubPath")
  {
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(szOutputFolder, "remove", "toplevel", ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2, "remove", "output2", ezFileSystem::AllowWrites) == EZ_SUCCESS);

    ezStringBuilder StartPath;
    ezStringBuilder SubPath;
    ezStringBuilder result, expected;

    // make sure this exists
    {
      ezFileWriter FileOut;
      EZ_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == EZ_SUCCESS);
    }


    {
      StartPath.Set(sOutputFolder1Resolved, "SubSub", "Irrelevant");
      SubPath.Set("DoesNotExist");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Failed());
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "SubSub", "Irrelevant");
      SubPath.Set("SubFolder2");
      expected.Set(sOutputFolderResolved, "/FoundationTest/IO/");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Succeeded());
      EZ_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/FoundationTest/");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Succeeded());
      EZ_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "SubSub");
      SubPath.Set("FoundationTest/IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Succeeded());
      EZ_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "SubSub", "Irrelevant");
      SubPath.Set("SubFolder2/FileSystemTest2.txt");
      expected.Set(sOutputFolderResolved, "/FoundationTest/IO/");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Succeeded());
      EZ_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(":toplevel/FoundationTest/IO/SubFolder");
      SubPath.Set("IO/SubFolder2");
      expected.Set(":toplevel/FoundationTest/");

      EZ_TEST_BOOL(ezFileSystem::FindFolderWithSubPath(StartPath, SubPath, result).Succeeded());
      EZ_TEST_STRING(result, expected);
    }

    ezFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    ezFileSystem::DeleteFile(":output2/FileSystemTest2.txt");

    ezFileSystem::RemoveDataDirectoryGroup("remove");
  }
}
