#include <FoundationTestPCH.h>

#include <Foundation/IO/OSFile.h>

EZ_CREATE_SIMPLE_TEST(IO, OSFile)
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

  const ezUInt32 uiTextLen = sFileContent.GetElementCount();

  ezStringBuilder sOutputFile = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO", "SubFolder");
  sOutputFile.AppendPath("OSFile_TestFile.txt");

  ezStringBuilder sOutputFile2 = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile2.MakeCleanPath();
  sOutputFile2.AppendPath("IO", "SubFolder2");
  sOutputFile2.AppendPath("OSFile_TestFileCopy.txt");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write File")
  {
    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile.GetData(), ezFileMode::Write) == EZ_SUCCESS);
    EZ_TEST_BOOL(f.IsOpen());
    EZ_TEST_INT(f.GetFilePosition(), 0);
    EZ_TEST_INT(f.GetFileSize(), 0);

    for (ezUInt32 i = 0; i < uiTextLen; ++i)
    {
      EZ_TEST_BOOL(f.Write(&sFileContent.GetData()[i], 1) == EZ_SUCCESS);
      EZ_TEST_INT(f.GetFilePosition(), i + 1);
      EZ_TEST_INT(f.GetFileSize(), i + 1);
    }

    EZ_TEST_INT(f.GetFilePosition(), uiTextLen);
    f.SetFilePosition(5, ezFilePos::FromStart);
    EZ_TEST_INT(f.GetFileSize(), uiTextLen);

    EZ_TEST_INT(f.GetFilePosition(), 5);
    // f.Close(); // The file should be closed automatically
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append File")
  {
    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile.GetData(), ezFileMode::Append) == EZ_SUCCESS);
    EZ_TEST_BOOL(f.IsOpen());
    EZ_TEST_INT(f.GetFilePosition(), uiTextLen);
    EZ_TEST_BOOL(f.Write(sFileContent.GetData(), uiTextLen) == EZ_SUCCESS);
    EZ_TEST_INT(f.GetFilePosition(), uiTextLen * 2);
    f.Close();
    EZ_TEST_BOOL(!f.IsOpen());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Read File")
  {
    const ezUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile.GetData(), ezFileMode::Read) == EZ_SUCCESS);
    EZ_TEST_BOOL(f.IsOpen());
    EZ_TEST_INT(f.GetFilePosition(), 0);

    EZ_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);
    EZ_TEST_INT(f.GetFilePosition(), uiTextLen * 2);

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy File")
  {
    ezOSFile::CopyFile(sOutputFile.GetData(), sOutputFile2.GetData());

    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile2.GetData(), ezFileMode::Read) == EZ_SUCCESS);

    const ezUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    EZ_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadAll")
  {
    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile, ezFileMode::Read) == EZ_SUCCESS);

    ezDynamicArray<ezUInt8> fileContent;
    const ezUInt64 bytes = f.ReadAll(fileContent);

    EZ_TEST_INT(bytes, uiTextLen * 2);

    EZ_TEST_BOOL(ezMemoryUtils::IsEqual(fileContent.GetData(), (const ezUInt8*)sFileContent.GetData(), uiTextLen));

    f.Close();
  }

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "File Stats")
  {
    ezFileStats s;

    ezStringBuilder dir = sOutputFile2.GetFileDirectory();

    EZ_TEST_BOOL(ezOSFile::GetFileStats(sOutputFile2.GetData(), s) == EZ_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Microsecond));

    EZ_TEST_BOOL(ezOSFile::GetFileStats(dir.GetData(), s) == EZ_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(ezSIUnitOfTime::Microsecond));
  }

#if (EZ_ENABLED(EZ_SUPPORTS_CASE_INSENSITIVE_PATHS) && EZ_ENABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS))
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileCasing")
  {
    ezStringBuilder dir = sOutputFile2;
    dir.ToLower();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // On Windows the drive letter will always be turned upper case by ezOSFile::GetFileCasing()
    // ensure that our input data ('ground truth') also uses an upper case drive letter
    auto driveLetterIterator = sOutputFile2.GetIteratorFront();
    const ezUInt32 uiDriveLetter = ezStringUtils::ToUpperChar(driveLetterIterator.GetCharacter());
    sOutputFile2.ChangeCharacter(driveLetterIterator, uiDriveLetter);
#endif

    ezStringBuilder sCorrected;
    EZ_TEST_BOOL(ezOSFile::GetFileCasing(dir.GetData(), sCorrected) == EZ_SUCCESS);

    // On Windows the drive letter will always be made to upper case
    EZ_TEST_STRING(sCorrected.GetData(), sOutputFile2.GetData());
  }
#endif // EZ_SUPPORTS_CASE_INSENSITIVE_PATHS && EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // EZ_SUPPORTS_FILE_STATS

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "File Iterator")
  {
    // It is not really possible to test this stuff (with a guaranteed result), as long as we do not have
    // a test data folder with deterministic content
    // Therefore I tested it manually, and leave the code in, such that it is at least a 'does it compile and link' test.

    ezStringBuilder sOutputFolder = ezOSFile::GetApplicationDirectory();
    sOutputFolder.AppendPath("*");

    ezStringBuilder sFullPath;

    ezUInt32 uiFolders = 0;
    ezUInt32 uiFiles = 0;

    ezFileSystemIterator it;
    if (it.StartSearch(sOutputFolder.GetData(), true, true) == EZ_SUCCESS)
    {
      int SkipFolder = 0;

      do
      {
        sFullPath = it.GetCurrentPath();
        sFullPath.AppendPath(it.GetStats().m_sName.GetData());

        it.GetStats();
        it.GetCurrentPath();

        if (it.GetStats().m_bIsDirectory)
        {
          ++uiFolders;

          SkipFolder++;

          if ((SkipFolder % 2 == 0) && (it.SkipFolder() == EZ_FAILURE))
            break;
        }
        else
        {
          ++uiFiles;
        }

        // printf("%s: '%s'\n", it.GetStats().m_bIsDirectory ? "[Dir] " : "[File]", sFullPath.GetData());
      } while (it.Next() == EZ_SUCCESS);
    }

    EZ_TEST_BOOL(uiFolders > 0);
    EZ_TEST_BOOL(uiFiles > 0);
  }

#endif

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete File")
  {
    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile.GetData()) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile.GetData()) == EZ_SUCCESS); // second time should still 'succeed'

    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile2.GetData()) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile2.GetData()) == EZ_SUCCESS); // second time should still 'succeed'

    ezOSFile f;
    EZ_TEST_BOOL(f.Open(sOutputFile.GetData(), ezFileMode::Read) == EZ_FAILURE);  // file should not exist anymore
    EZ_TEST_BOOL(f.Open(sOutputFile2.GetData(), ezFileMode::Read) == EZ_FAILURE); // file should not exist anymore
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExistsFile")
  {
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile.GetData()) == false);
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      ezOSFile f;
      EZ_TEST_BOOL(f.Open(sOutputFile.GetData(), ezFileMode::Write) == EZ_SUCCESS);
    }

    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile.GetData()) == true);
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      ezOSFile f;
      EZ_TEST_BOOL(f.Open(sOutputFile2.GetData(), ezFileMode::Write) == EZ_SUCCESS);
    }

    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile.GetData()) == true);
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile2.GetData()) == true);

    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile.GetData()) == EZ_SUCCESS);
    EZ_TEST_BOOL(ezOSFile::DeleteFile(sOutputFile2.GetData()) == EZ_SUCCESS);

    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile.GetData()) == false);
    EZ_TEST_BOOL(ezOSFile::ExistsFile(sOutputFile2.GetData()) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExistsDirectory")
  {
    // files are not folders
    EZ_TEST_BOOL(ezOSFile::ExistsDirectory(sOutputFile.GetData()) == false);
    EZ_TEST_BOOL(ezOSFile::ExistsDirectory(sOutputFile2.GetData()) == false);

    ezStringBuilder sOutputFolder = ezTestFramework::GetInstance()->GetAbsOutputPath();
    EZ_TEST_BOOL(ezOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("IO");
    EZ_TEST_BOOL(ezOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("SubFolder");
    EZ_TEST_BOOL(ezOSFile::ExistsDirectory(sOutputFolder) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetApplicationDirectory")
  {
    const char* szAppDir = ezOSFile::GetApplicationDirectory();
    EZ_IGNORE_UNUSED(szAppDir);
  }
}
