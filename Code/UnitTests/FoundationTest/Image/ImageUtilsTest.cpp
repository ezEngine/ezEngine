#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/ImageUtils.h>


EZ_CREATE_SIMPLE_TEST(Image, ImageUtils)
{
  ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
  ezStringBuilder sWriteDir = ezTestFramework::GetInstance()->GetAbsOutputPath();

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezResult addDir = ezFileSystem::AddDataDirectory(sReadDir.GetData(), "ImageTest");
  EZ_TEST_BOOL(addDir == EZ_SUCCESS);

  if (addDir.Failed())
    return;

  addDir = ezFileSystem::AddDataDirectory(sWriteDir.GetData(), "ImageTest", "output", ezFileSystem::AllowWrites);
  EZ_TEST_BOOL(addDir == EZ_SUCCESS);

  if (addDir.Failed())
    return;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifferenceABS RGB")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    ezImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGB.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGB.tga", "ImageUtils/Diff_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeImageDifferenceABS RGBA")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGBA.tga").IgnoreResult();

    ezImageUtils::ComputeImageDifferenceABS(ImageA, ImageB, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/Diff_RGBA.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedDiff_RGBA.tga", "ImageUtils/Diff_RGBA.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaledown Half RGB")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ezImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGB.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGB.tga", "ImageUtils/ScaledHalf_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaledown Half RGBA")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ezImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();

    ImageAc.SaveTo(":output/ImageUtils/ScaledHalf_RGBA.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedScaledHalf_RGBA.tga", "ImageUtils/ScaledHalf_RGBA.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CropImage RGB")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ezImageUtils::CropImage(ImageA, ezVec2I32(100, 50), ezSizeU32(300, 200), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGB.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedCrop_RGB.tga", "ImageUtils/Crop_RGB.tga", "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CropImage RGBA")
  {
    ezImage ImageA, ImageAc;
    ImageA.LoadFrom("ImageUtils/ImageA_RGBA.tga").IgnoreResult();
    ezImageUtils::CropImage(ImageA, ezVec2I32(100, 75), ezSizeU32(300, 180), ImageAc);

    ImageAc.SaveTo(":output/ImageUtils/Crop_RGBA.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedCrop_RGBA.tga", "ImageUtils/Crop_RGBA.tga", "");
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeMeanSquareError")
  {
    ezImage ImageA, ImageB, ImageDiff;
    ImageA.LoadFrom("ImageUtils/ImageA_RGB.tga").IgnoreResult();
    ImageB.LoadFrom("ImageUtils/ImageB_RGB.tga").IgnoreResult();

    ezImage ImageAc, ImageBc;
    ezImageUtils::Scale(ImageA, ImageAc, ImageA.GetWidth() / 2, ImageA.GetHeight() / 2).IgnoreResult();
    ezImageUtils::Scale(ImageB, ImageBc, ImageB.GetWidth() / 2, ImageB.GetHeight() / 2).IgnoreResult();

    ezImageUtils::ComputeImageDifferenceABS(ImageAc, ImageBc, ImageDiff);

    ImageDiff.SaveTo(":output/ImageUtils/MeanSquareDiff_RGB.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedMeanSquareDiff_RGB.tga", "ImageUtils/MeanSquareDiff_RGB.tga", "");

    ezUInt32 uiError = ezImageUtils::ComputeMeanSquareError(ImageDiff, 4);
    EZ_TEST_INT(uiError, 1433);
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
