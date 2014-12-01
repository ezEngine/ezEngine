#include <PCH.h>
#include <CoreUtils/Image/ImageUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>


EZ_CREATE_SIMPLE_TEST(Image, ImageUtils)
{
  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/UnitTests/CoreTest");

  ezStringBuilder sWriteDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sWriteDir.AppendPath("CoreTest");

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "ImageTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::AllowWrites, "ImageTest") == EZ_SUCCESS);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifference RGBA")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga");
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga");

    ezImageUtils::ComputeImageDifference(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo("ImageUtils/Diff_RGBA.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifference RGB")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga");
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga");

    ezImageUtils::ComputeImageDifference(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo("ImageUtils/Diff_RGB.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGB.tga", "ImageUtils/Diff_RGB.tga", "");
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}

