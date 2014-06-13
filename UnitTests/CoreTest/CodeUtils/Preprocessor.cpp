#include <PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>


EZ_CREATE_SIMPLE_TEST_GROUP(CodeUtils);

static void Test(const char* szFile)
{

}

ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent)
{
  ezFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return EZ_FAILURE;

  ezUInt8 Temp[4096];

  while (ezUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32) uiRead));
  }

  return EZ_SUCCESS;
}

ezResult FileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType IncType, ezString& out_sAbsoluteFilePath)
{
  ezStringBuilder s;

  if (IncType == ezPreprocessor::RelativeInclude)
  {
    s = szCurAbsoluteFile;
    s.AppendPath(szIncludeFile);
    s.MakeCleanPath();
  }
  else
  {
    s = szIncludeFile;
    s.MakeCleanPath();
  }

  out_sAbsoluteFilePath = s;
  return EZ_SUCCESS;
}

EZ_CREATE_SIMPLE_TEST(CodeUtils, Preprocessor)
{
  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/UnitTests/CoreTest");

  ezStringBuilder sWriteDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sWriteDir.AppendPath("CoreTest");

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "PreprocessorTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::AllowWrites, "PreprocessorTest") == EZ_SUCCESS);

  ezTokenizedFileCache SharedCache;

  {
    const char* szTestFiles[] =
    {
      "Empty",
      "Test1", 
    };

    ezStringBuilder sOutput;
    ezStringBuilder fileName;
    ezStringBuilder fileNameOut;
    ezStringBuilder fileNameExp;

    for (int i = 0; i < EZ_ARRAY_SIZE(szTestFiles); i++)
    {
      EZ_TEST_BLOCK(ezTestBlock::Enabled, szTestFiles[i])
      {
        ezPreprocessor pp;
        pp.SetLogInterface(ezGlobalLog::GetInstance());
        pp.SetPassThroughLine(false);
        pp.SetPassThroughPragma(true);
        pp.SetFileCallbacks(FileOpen, FileLocator);
        pp.SetCustomFileCache(&SharedCache);

        {
          fileName.Format("Preprocessor/%s.txt", szTestFiles[i]);
          fileNameExp.Format("Preprocessor/%s - Expected.txt", szTestFiles[i]);

          EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "File does not exist: '%s'", fileName.GetData());
          EZ_TEST_BOOL_MSG(pp.Process(fileName.GetData(), sOutput) == EZ_SUCCESS, "Processing failed: '%s'", fileName.GetData());
        }

        {
          fileNameOut.Format("Preprocessor/%s - Result.txt", szTestFiles[i]);

          {
            ezFileWriter fout;
            if (fout.Open(fileNameOut.GetData()).Succeeded())
              fout.WriteBytes(sOutput.GetData(), sOutput.GetElementCount());
          }

          EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileNameOut.GetData()), "Output file is missing: '%s'", fileNameOut.GetData());
        }

        EZ_TEST_FILES(fileNameOut.GetData(), fileNameExp.GetData(), "");
      }
    }
  }


  ezFileSystem::RemoveDataDirectoryGroup("PreprocessorTest");
}


