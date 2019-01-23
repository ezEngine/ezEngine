#include <PCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

EZ_CREATE_SIMPLE_TEST(IO, DeferredFileWriter)
{
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory("", "", ":", ezFileSystem::AllowWrites) == EZ_SUCCESS);

  const ezStringBuilder szOutputFolder = ezTestFramework::GetInstance()->GetAbsOutputPath();
  ezStringBuilder sOutputFolderResolved;
  ezFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved);

  ezStringBuilder sTempFile = sOutputFolderResolved;
  sTempFile.AppendPath("Temp.tmp");

  // make sure the file does not exist
  ezFileSystem::DeleteFile(sTempFile);
  EZ_TEST_BOOL(!ezFileSystem::ExistsFile(sTempFile));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DeferredFileWriter")
  {
    ezDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (ezUInt64 i = 0; i < 1'000'000; ++i)
    {
      writer << i;
    }

    // does not exist yet
    EZ_TEST_BOOL(!ezFileSystem::ExistsFile(sTempFile));
  }

  // now it exists
  EZ_TEST_BOOL(ezFileSystem::ExistsFile(sTempFile));

  // check content is correct
  {
    ezFileReader reader;
    EZ_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (ezUInt64 i = 0; i < 1'000'000; ++i)
    {
      ezUInt64 v;
      reader >> v;
      EZ_TEST_BOOL(v == i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "DeferredFileWriter2")
  {
    ezDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (ezUInt64 i = 1; i < 100'000; ++i)
    {
      writer << i;
    }

    // does exist from earlier
    EZ_TEST_BOOL(ezFileSystem::ExistsFile(sTempFile));

    // check content is as previous correct
    {
      ezFileReader reader;
      EZ_TEST_BOOL(reader.Open(sTempFile).Succeeded());

      for (ezUInt64 i = 0; i < 1'000'000; ++i)
      {
        ezUInt64 v;
        reader >> v;
        EZ_TEST_BOOL(v == i);
      }
    }
  }

  // exist but now was overwritten
  EZ_TEST_BOOL(ezFileSystem::ExistsFile(sTempFile));

  // check content is as previous correct
  {
    ezFileReader reader;
    EZ_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (ezUInt64 i = 1; i < 100'000; ++i)
    {
      ezUInt64 v;
      reader >> v;
      EZ_TEST_BOOL(v == i);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Discard")
  {
    ezStringBuilder sTempFile2 = sOutputFolderResolved;
    sTempFile2.AppendPath("Temp2.tmp");
    {
      ezDeferredFileWriter writer;
      writer.SetOutput(sTempFile2);
      writer << 10;
      writer.Discard();
    }
    EZ_TEST_BOOL(!ezFileSystem::ExistsFile(sTempFile2));
  }

  ezFileSystem::DeleteFile(sTempFile);
  ezFileSystem::ClearAllDataDirectories();
}
