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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifferenceABS RGB")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga");
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga");

    ezImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo("ImageUtils/Diff_RGB.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGB.tga", "ImageUtils/Diff_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifferenceABS RGBA")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga");
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga");

    ezImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo("ImageUtils/Diff_RGBA.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaledown Half RGB")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga");
    ezImageUtils::ScaleDownHalf(ImageA, ImageAc);

    ImageAc.SaveTo("ImageUtils/ScaledHalf_RGB.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGB.tga", "ImageUtils/ScaledHalf_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaledown Half RGBA")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga");
    ezImageUtils::ScaleDownHalf(ImageA, ImageAc);

    ImageAc.SaveTo("ImageUtils/ScaledHalf_RGBA.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGBA.tga", "ImageUtils/ScaledHalf_RGBA.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CropImage RGB")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga");
    ezImageUtils::CropImage(ImageA, ezVec2I32(100, 50), ezSizeU32(300, 200), ImageAc);

    ImageAc.SaveTo("ImageUtils/Crop_RGB.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedCrop_RGB.tga", "ImageUtils/Crop_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CropImage RGBA")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga");
    ezImageUtils::CropImage(ImageA, ezVec2I32(100, 75), ezSizeU32(300, 180), ImageAc);

    ImageAc.SaveTo("ImageUtils/Crop_RGBA.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedCrop_RGBA.tga", "ImageUtils/Crop_RGBA.tga", "");
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeMeanSquareError")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga");
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga");

    ezImage ImageAc, ImageBc;
    ezImageUtils::ScaleDownHalf(ImageA, ImageAc);
    ezImageUtils::ScaleDownHalf(ImageB, ImageBc);

    ezImageUtils::ComputeImageDifferenceABS(ImageAc, ImageBc, ImageDiff);

    ImageDiff.SaveTo("ImageUtils/MeanSquareDiff_RGB.tga");

    EZ_TEST_FILES("ImageUtils/ExpectedMeanSquareDiff_RGB.tga", "ImageUtils/MeanSquareDiff_RGB.tga", "");

    ezUInt32 uiError = ezImageUtils::ComputeMeanSquareError(ImageDiff, 4);
    EZ_TEST_INT(uiError, 2004);
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}

