#include <FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Texture/Image/Image.h>

EZ_CREATE_SIMPLE_TEST(SimdMath, SimdNoise)
{
  ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
  ezStringBuilder sWriteDir = ezTestFramework::GetInstance()->GetAbsOutputPath();

  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir, "SimdNoise") == EZ_SUCCESS);
  EZ_TEST_BOOL_MSG(ezFileSystem::AddDataDirectory(sWriteDir, "SimdNoise", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS,
    "Failed to mount data dir '%s'", sWriteDir.GetData());

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Perlin")
  {
    const ezUInt32 uiSize = 128;

    ezImageHeader imageHeader;
    imageHeader.SetWidth(uiSize);
    imageHeader.SetHeight(uiSize);
    imageHeader.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);

    ezImage image;
    image.ResetAndAlloc(imageHeader);

    ezSimdPerlinNoise perlin(12345);
    ezSimdVec4f xOffset(0, 1, 2, 3);
    ezSimdFloat scale(100);

    for (ezUInt32 uiNumOctaves = 1; uiNumOctaves <= 6; ++uiNumOctaves)
    {
      ezColorLinearUB* data = image.GetPixelPointer<ezColorLinearUB>();
      for (ezUInt32 y = 0; y < uiSize; ++y)
      {
        for (ezUInt32 x = 0; x < uiSize / 4; ++x)
        {
          ezSimdVec4f sX = (ezSimdVec4f(x * 4) + xOffset) / scale;
          ezSimdVec4f sY = ezSimdVec4f(y) / scale;

          ezSimdVec4f noise = perlin.NoiseZeroToOne(sX, sY, ezSimdVec4f::ZeroVector(), uiNumOctaves);
          float p[4];
          p[0] = noise.x();
          p[1] = noise.y();
          p[2] = noise.z();
          p[3] = noise.w();

          ezUInt32 uiPixelIndex = y * uiSize + x * 4;
          for (ezUInt32 i = 0; i < 4; ++i)
          {
            data[uiPixelIndex + i] = ezColor(p[i], p[i], p[i]);
          }
        }
      }

      ezStringBuilder sOutFile;
      sOutFile.Format(":output/SimdNoise/perlin_{}.tga", uiNumOctaves);

      EZ_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      ezStringBuilder sInFile;
      sInFile.Format("SimdNoise/perlin_{}.tga", uiNumOctaves);
      EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      EZ_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}
