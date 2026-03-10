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

  addDir = ezFileSystem::AddDataDirectory(sWriteDir.GetData(), "ImageTest", "output", ezDataDirUsage::AllowWrites);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Scaleup 16x RGBA")
  {
    ezImage ImageA, ImageUpscaled;
    ImageA.LoadFrom("ImageUtils/ColoredChecker.tga").IgnoreResult();

    ezImageUtils::Scale(ImageA, ImageUpscaled, ImageA.GetWidth() * 16, ImageA.GetHeight() * 16, nullptr, ezImageAddressMode::Repeat, ezImageAddressMode::Repeat).IgnoreResult();

    ImageUpscaled.SaveTo(":output/ImageUtils/ColoredChecker16x.tga").IgnoreResult();

    EZ_TEST_FILES("ImageUtils/ExpectedColoredChecker16x.tga", "ImageUtils/ColoredChecker16x.tga", "");
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CopyChannel")
  {
    const ezUInt32 uiWidth = 4;
    const ezUInt32 uiHeight = 2;
    const ezUInt32 uiNumPixels = uiWidth * uiHeight;

    // Helper to test CopyChannel between a source and destination format with potentially different channel counts.
    // Fills the source with a known pattern, copies one channel to the destination, and verifies.
    auto TestCopyChannel = [&](ezImageFormat::Enum srcFormat, ezImageFormat::Enum dstFormat, auto typeTag)
    {
      using T = decltype(typeTag);

      const ezUInt32 uiSrcChannels = ezImageFormat::GetNumChannels(srcFormat);
      const ezUInt32 uiDstChannels = ezImageFormat::GetNumChannels(dstFormat);

      for (ezUInt8 srcCh = 0; srcCh < uiSrcChannels; ++srcCh)
      {
        for (ezUInt8 dstCh = 0; dstCh < uiDstChannels; ++dstCh)
        {
          ezImageHeader srcHeader;
          srcHeader.SetWidth(uiWidth);
          srcHeader.SetHeight(uiHeight);
          srcHeader.SetImageFormat(srcFormat);

          ezImageHeader dstHeader;
          dstHeader.SetWidth(uiWidth);
          dstHeader.SetHeight(uiHeight);
          dstHeader.SetImageFormat(dstFormat);

          ezImage srcImg, dstImg;
          srcImg.ResetAndAlloc(srcHeader);
          dstImg.ResetAndAlloc(dstHeader);

          // Fill source: channel value = (pixelIndex * srcChannels + channelIndex) so each value is unique.
          T* pSrc = srcImg.GetPixelPointer<T>();
          for (ezUInt32 px = 0; px < uiNumPixels; ++px)
          {
            for (ezUInt32 ch = 0; ch < uiSrcChannels; ++ch)
            {
              pSrc[px * uiSrcChannels + ch] = static_cast<T>(px * uiSrcChannels + ch + 1);
            }
          }

          // Fill dest with a constant sentinel value.
          const T sentinel = static_cast<T>(255);
          T* pDst = dstImg.GetPixelPointer<T>();
          for (ezUInt32 i = 0; i < uiNumPixels * uiDstChannels; ++i)
          {
            pDst[i] = sentinel;
          }

          EZ_TEST_BOOL(ezImageUtils::CopyChannel(dstImg, dstCh, srcImg, srcCh).Succeeded());

          // Verify: only dstCh should have changed; other channels remain sentinel.
          for (ezUInt32 px = 0; px < uiNumPixels; ++px)
          {
            for (ezUInt32 ch = 0; ch < uiDstChannels; ++ch)
            {
              T actual = pDst[px * uiDstChannels + ch];
              if (ch == dstCh)
              {
                T expected = static_cast<T>(px * uiSrcChannels + srcCh + 1);
                EZ_TEST_BOOL(actual == expected);
              }
              else
              {
                EZ_TEST_BOOL(actual == sentinel);
              }
            }
          }
        }
      }
    };

    // Shorthand for same-format tests.
    auto TestSameFormat = [&](ezImageFormat::Enum format, auto typeTag)
    {
      TestCopyChannel(format, format, typeTag);
    };

    // 8-bit UNORM: same-format (stride 1-4)
    TestSameFormat(ezImageFormat::R8_UNORM, ezUInt8{});
    TestSameFormat(ezImageFormat::R8G8_UNORM, ezUInt8{});
    TestSameFormat(ezImageFormat::R8G8B8_UNORM, ezUInt8{});
    TestSameFormat(ezImageFormat::R8G8B8A8_UNORM, ezUInt8{});

    // 16-bit UNORM: same-format (stride 1-4)
    TestSameFormat(ezImageFormat::R16_UNORM, ezUInt16{});
    TestSameFormat(ezImageFormat::R16G16_UNORM, ezUInt16{});
    TestSameFormat(ezImageFormat::R16G16B16_UNORM, ezUInt16{});
    TestSameFormat(ezImageFormat::R16G16B16A16_UNORM, ezUInt16{});

    // 32-bit float: same-format (stride 1-4)
    TestSameFormat(ezImageFormat::R32_FLOAT, ezUInt32{});
    TestSameFormat(ezImageFormat::R32G32_FLOAT, ezUInt32{});
    TestSameFormat(ezImageFormat::R32G32B32_FLOAT, ezUInt32{});
    TestSameFormat(ezImageFormat::R32G32B32A32_FLOAT, ezUInt32{});

    // 8-bit UNORM: cross-format (different src/dst channel counts)
    TestCopyChannel(ezImageFormat::R8_UNORM, ezImageFormat::R8G8B8A8_UNORM, ezUInt8{});
    TestCopyChannel(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8_UNORM, ezUInt8{});
    TestCopyChannel(ezImageFormat::R8G8_UNORM, ezImageFormat::R8G8B8_UNORM, ezUInt8{});
    TestCopyChannel(ezImageFormat::R8G8B8_UNORM, ezImageFormat::R8G8_UNORM, ezUInt8{});
    TestCopyChannel(ezImageFormat::R8_UNORM, ezImageFormat::R8G8_UNORM, ezUInt8{});
    TestCopyChannel(ezImageFormat::R8G8B8A8_UNORM, ezImageFormat::R8G8B8_UNORM, ezUInt8{});

    // 16-bit UNORM: cross-format
    TestCopyChannel(ezImageFormat::R16_UNORM, ezImageFormat::R16G16B16A16_UNORM, ezUInt16{});
    TestCopyChannel(ezImageFormat::R16G16B16A16_UNORM, ezImageFormat::R16_UNORM, ezUInt16{});
    TestCopyChannel(ezImageFormat::R16G16_UNORM, ezImageFormat::R16G16B16_UNORM, ezUInt16{});

    // 32-bit float: cross-format
    TestCopyChannel(ezImageFormat::R32_FLOAT, ezImageFormat::R32G32B32A32_FLOAT, ezUInt32{});
    TestCopyChannel(ezImageFormat::R32G32B32A32_FLOAT, ezImageFormat::R32_FLOAT, ezUInt32{});
    TestCopyChannel(ezImageFormat::R32G32_FLOAT, ezImageFormat::R32G32B32_FLOAT, ezUInt32{});
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CopyChannel Failure Cases")
  {
    ezImageHeader header4x4;
    header4x4.SetWidth(4);
    header4x4.SetHeight(4);

    // Mismatched dimensions
    {
      ezImageHeader headerSmall;
      headerSmall.SetWidth(2);
      headerSmall.SetHeight(4);
      headerSmall.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

      ezImageHeader headerBig;
      headerBig.SetWidth(4);
      headerBig.SetHeight(4);
      headerBig.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

      ezImage src, dst;
      src.ResetAndAlloc(headerSmall);
      dst.ResetAndAlloc(headerBig);
      EZ_TEST_BOOL(ezImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Mismatched bit depth (8-bit vs 32-bit) should fail
    {
      ezImageHeader header8;
      header8.SetWidth(4);
      header8.SetHeight(4);
      header8.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

      ezImageHeader header32;
      header32.SetWidth(4);
      header32.SetHeight(4);
      header32.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);

      ezImage src, dst;
      src.ResetAndAlloc(header8);
      dst.ResetAndAlloc(header32);
      EZ_TEST_BOOL(ezImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Mismatched type (unorm vs snorm) should fail
    {
      ezImageHeader headerUnorm;
      headerUnorm.SetWidth(4);
      headerUnorm.SetHeight(4);
      headerUnorm.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

      ezImageHeader headerSnorm;
      headerSnorm.SetWidth(4);
      headerSnorm.SetHeight(4);
      headerSnorm.SetImageFormat(ezImageFormat::R8G8B8A8_SNORM);

      ezImage src, dst;
      src.ResetAndAlloc(headerUnorm);
      dst.ResetAndAlloc(headerSnorm);
      EZ_TEST_BOOL(ezImageUtils::CopyChannel(dst, 0, src, 0).Failed());
    }

    // Channel index out of range
    {
      header4x4.SetImageFormat(ezImageFormat::R8G8_UNORM);
      ezImage src, dst;
      src.ResetAndAlloc(header4x4);
      dst.ResetAndAlloc(header4x4);
      EZ_TEST_BOOL(ezImageUtils::CopyChannel(dst, 2, src, 0).Failed());
      EZ_TEST_BOOL(ezImageUtils::CopyChannel(dst, 0, src, 2).Failed());
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
