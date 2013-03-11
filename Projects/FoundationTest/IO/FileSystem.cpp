#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirType_Folder.h>

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

  ezStringBuilder sOutputFolder1 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder1.AppendPath("FoundationTest", "SubFolder");

  ezStringBuilder sOutputFolder2 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder2.AppendPath("FoundationTest", "SubFolder2");


  EZ_TEST_BLOCK(true, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectoryType_Folder::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectoryType_Folder::Factory);
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectoryType_Folder::Factory);

    ezFileSystem::ClearAllDataDirectoryFactories();

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectoryType_Folder::Factory);

    EZ_TEST(ezFileSystem::AddDataDirectory(BUILDSYSTEM_OUTPUT_FOLDER, ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);

    ezStringBuilder sTempFile = sOutputFolder1;
    sTempFile.AppendPath("Temp.tmp");

    ezFileWriter TempFile;
    EZ_TEST(TempFile.Open(sTempFile.GetData()) == EZ_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2;
    sTempFile.AppendPath("Temp.tmp");

    EZ_TEST(TempFile.Open(sTempFile.GetData()) == EZ_SUCCESS);
    TempFile.Close();

    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites, "Clear") == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Clear") == EZ_SUCCESS);

    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);

    EZ_TEST(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 3);

    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly, "Remove") == EZ_SUCCESS);

    ezFileSystem::ClearAllDataDirectories();

    EZ_TEST(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 0);
    EZ_TEST(ezFileSystem::RemoveDataDirectoryGroup("Clear") == 0);

    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites) == EZ_SUCCESS);
    EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::ReadOnly) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(true, "Write File")
  {
    ezFileWriter FileOut;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST(FileOut.Open("FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST(FileOut.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST(FileOut.GetFilePathAbsolute() == sAbs.GetData());

    EZ_TEST(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == EZ_SUCCESS);

    FileOut.Close();
  }

  EZ_TEST_BLOCK(true, "Read File")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST(FileIn.Open("FileSystemTest.txt") == EZ_SUCCESS);

    EZ_TEST(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST(FileIn.GetFilePathAbsolute() == sAbs.GetData());

    char szTemp[1024 * 2];
    EZ_TEST(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  EZ_TEST_BLOCK(true, "Read File (Absolute Path)")
  {
    ezFileReader FileIn;

    ezStringBuilder sAbs = sOutputFolder1;
    sAbs.AppendPath("FileSystemTest.txt");

    EZ_TEST(FileIn.Open(sAbs.GetData()) == EZ_SUCCESS);

    EZ_TEST(FileIn.GetFilePathRelative() == "FileSystemTest.txt");
    EZ_TEST(FileIn.GetFilePathAbsolute() == sAbs.GetData());

    char szTemp[1024 * 2];
    EZ_TEST(FileIn.ReadBytes(szTemp, 1024 * 2) == sFileContent.GetElementCount());

    EZ_TEST(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

  EZ_TEST_BLOCK(true, "Delete File / Exists File")
  {
    EZ_TEST(ezFileSystem::ExistsFile("FileSystemTest.txt"));
    ezFileSystem::DeleteFile("FileSystemTest.txt");
    EZ_TEST(!ezFileSystem::ExistsFile("FileSystemTest.txt"));

    ezFileReader FileIn;
    EZ_TEST(FileIn.Open("FileSystemTest.txt") == EZ_FAILURE);
  }

  EZ_TEST_BLOCK(true, "Find Valid Path")
  {
    ezString sRel, sAbs;

    EZ_TEST(ezFileSystem::ResolvePath("FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

    ezStringBuilder sExpectedAbs = sOutputFolder1;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    EZ_TEST(sAbs == sExpectedAbs);
    EZ_TEST(sRel == "FileSystemTest2.txt");

    // create a file in the second dir
    {
      EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder2.GetData(), ezFileSystem::AllowWrites, "Remove") == EZ_SUCCESS);

      {
        ezFileWriter FileOut;
        EZ_TEST(FileOut.Open("FileSystemTest2.txt") == EZ_SUCCESS);
      }

      EZ_TEST(ezFileSystem::RemoveDataDirectoryGroup("Remove") == 1);
    }

    // find the path to an existing file
    {
      EZ_TEST(ezFileSystem::ResolvePath("FileSystemTest2.txt", false, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder2;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST(sAbs == sExpectedAbs);
      EZ_TEST(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST(ezFileSystem::ResolvePath("FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      EZ_TEST(sAbs == sExpectedAbs);
      EZ_TEST(sRel == "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      EZ_TEST(ezFileSystem::ResolvePath("SubSub/FileSystemTest2.txt", true, &sAbs, &sRel) == EZ_SUCCESS);

      sExpectedAbs = sOutputFolder1;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      EZ_TEST(sAbs == sExpectedAbs);
      EZ_TEST(sRel == "SubSub/FileSystemTest2.txt");
    }

    ezFileSystem::DeleteFile("FileSystemTest2.txt");
  }
}
