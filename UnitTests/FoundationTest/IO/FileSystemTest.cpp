#include <PCH.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

EZ_CREATE_SIMPLE_TEST(IO, FileSystem)
{
  ezStringBuilder sFileContent = "Lyrics to Taste The Cake:\n\
Turret: Who's there?\n\
Turret: Is anyone there?\n\
Turret: I see you.\n\
\n\
Chell rises from a stasis inside of a glass box\n\
She isn't greeted by faces,\n\
Only concrete and clocks.\n\
...";

  // Don't use 'BUILDSYSTEM_OUTPUT_FOLDER' in any other tests, it's just used here because we are too poor to get
  // an absolute path that actually exists from anywhere else.
  ezStringBuilder sOutputFolder1 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder1.AppendPath("FoundationTest", "IO", "SubFolder");

  ezStringBuilder sOutputFolder2 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder2.AppendPath("FoundationTest", "IO", "SubFolder2");


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    ezFileSystem::ClearAllDataDirectoryFactories();

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(BUILDSYSTEM_OUTPUT_FOLDER, ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);

    ezStringBuilder sTempFile = sOutputFolder1;
    sTempFile.AppendPath("Temp.tmp");

    ezFileWriter TempFile;
    EZ_TEST_BOOL(TempFile.Open(sTempFile.GetData()) == EZ_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2;
    sTempFile.AppendPath("Temp.tmp");

    EZ_TEST_BOOL(TempFile.Open(sTempFile.GetData()) == EZ_SUCCESS);
    TempFile.Close();

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Clear") == EZ_SUCCESS);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);

    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 3);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);

    ezFileSystem::ClearAllDataDirectories();

    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 0);
    EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Clear") == 0);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write File")
  {
    ezFileWriter FileOut;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileOut.Open("FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST_BOOL(FileOut.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileOut.GetFilePathAbsolute() == sAbs.GetData());

    EZ_TEST_BOOL(FileOut.GetFileSize() == 0);

    EZ_TEST_BOOL(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == EZ_SUCCESS);

    FileOut.Flush();
    EZ_TEST_BOOL(FileOut.GetFileSize() == sFileContent.GetElementCount());

    FileOut.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read File")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST_BOOL(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileIn.GetFilePathAbsolute() == sAbs.GetData());

    EZ_TEST_BOOL(FileIn.GetFileSize() == sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    EZ_TEST_BOOL(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read File (Absolute Path)")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST_BOOL(FileIn.Open(sAbs.GetData()) == EZ_SUCCESS);

    EZ_TEST_BOOL(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST_BOOL(FileIn.GetFilePathAbsolute() == sAbs.GetData());

    EZ_TEST_BOOL(FileIn.GetFileSize() == sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    EZ_TEST_BOOL(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete File / Exists File")
  {
    EZ_TEST_BOOL(ezFileSystem::ExistsFile("FileSystemTest.txt"));
    ezFileSystem::DeleteFile("FileSystemTest.txt");
    EZ_TEST_BOOL(!ezFileSystem::ExistsFile("FileSystemTest.txt"));

    ezFileReader FileIn;
    EZ_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Find Valid Path")
  {
    ezString sRel, sAbs;

    EZ_TEST_BOOL(ezFileSystem::ResolvePath("FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

    ezStringBuilder sExpectedAbs = sOutputFolder1;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    EZ_TEST_BOOL(sAbs == sExpectedAbs);
    EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");

    // create a file in the second dir
    {
      EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);

      {
        ezFileWriter FileOut;
        EZ_TEST_BOOL(FileOut.Open("FileSystemTest2.txt") == EZ_SUCCESS);
      }

      EZ_TEST_BOOL(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 1);
    }

    // find the path to an existing file
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath("FileSystemTest2.txt", false, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder2;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath("FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST_BOOL(ezFileSystem::ResolvePath("SubSub/FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      EZ_TEST_BOOL(sAbs == sExpectedAbs);
      EZ_TEST_BOOL(sRel == "SubSub/FileSystemTest2.txt");
    }

    ezFileSystem::DeleteFile("FileSystemTest2.txt");
  }
}
