#include <FoundationTestPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST(IO, Archive)
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::ArchiveType::Factory);

  ezStringBuilder sOutputFolder = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("ArchiveTest");
  sOutputFolder.MakeCleanPath();

  // make sure it is empty
  ezOSFile::DeleteFolder(sOutputFolder);
  ezOSFile::CreateDirectoryStructure(sOutputFolder);

  if (EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS).Failed())
    return;

  const char* szTestData = "TestData";
  const char* szUnpackedData = "Unpacked";

  // write a couple of files for packaging
  const char* szFileList[] = {
    "File1.txt",
    "FolderA/File2.jpg", // should get stored uncompressed
    "FolderB/File3.txt",
    "FolderA/FolderC/File4.zip", // should get stored uncompressed
    "FolderA/FolderD/File5.txt",
    "File6.txt",
  };

  const ezUInt32 uiMinFileSize = 1024 * 128;


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Generate Data")
  {
    ezUInt64 uiValue = 0;

    ezStringBuilder fileName;

    for (ezUInt32 uiFileIdx = 0; uiFileIdx < EZ_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      fileName.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);

      ezFileWriter file;
      if (EZ_TEST_BOOL(file.Open(fileName).Succeeded()).Failed())
        return;

      for (ezUInt32 i = 0; i < uiMinFileSize * uiFileIdx; ++i)
      {
        file << uiValue;
        ++uiValue;
      }
    }
  }

  const ezStringBuilder sArchiveFolder(sOutputFolder, "/", szTestData);
  const ezStringBuilder sUnpackFolder(sOutputFolder, "/", szUnpackedData);
  const ezStringBuilder sArchiveFile(sOutputFolder, "/", szTestData, ".ezArchive");

  ezStringBuilder pathToArchiveTool = ezCommandLineUtils::GetGlobalInstance()->GetParameter(0);
  pathToArchiveTool.PathParentDirectory();
  pathToArchiveTool.AppendPath("ArchiveTool.exe");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Create a Package")
  {

    ezProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack(sArchiveFolder);

    ezInt32 iReturnValue = 1;

    ezProcess ArchiveToolProc;
    if (EZ_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()).Failed())
      return;

    EZ_TEST_INT(iReturnValue, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unpack the Package")
  {

    ezProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack("-unpack");
    opt.m_Arguments.PushBack(sArchiveFile);
    opt.m_Arguments.PushBack("-out");
    opt.m_Arguments.PushBack(sUnpackFolder);

    ezInt32 iReturnValue = 1;

    ezProcess ArchiveToolProc;
    if (EZ_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()).Failed())
      return;

    EZ_TEST_INT(iReturnValue, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare unpacked data")
  {
    ezUInt64 uiValue = 0;

    ezStringBuilder sFileSrc;
    ezStringBuilder sFileDst;

    for (ezUInt32 uiFileIdx = 0; uiFileIdx < EZ_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(sOutputFolder, "/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(sOutputFolder, "/", szUnpackedData, "/", szFileList[uiFileIdx]);

      EZ_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Mount as Data Dir")
  {
    if (EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive", ezFileSystem::ReadOnly) == EZ_SUCCESS).Failed())
      return;

    ezStringBuilder sFileSrc;
    ezStringBuilder sFileDst;

    // test opening multiple files in parallel and keeping them open
    ezFileReader readers[EZ_ARRAY_SIZE(szFileList)];
    for (ezUInt32 uiFileIdx = 0; uiFileIdx < EZ_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);
      EZ_TEST_BOOL(readers[uiFileIdx].Open(sFileDst).Succeeded());

      // advance the reader a bit
      EZ_TEST_INT(readers[uiFileIdx].SkipBytes(uiMinFileSize * uiFileIdx), uiMinFileSize * uiFileIdx);
    }

    for (ezUInt32 uiFileIdx = 0; uiFileIdx < EZ_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);

      EZ_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("Clear");
}
