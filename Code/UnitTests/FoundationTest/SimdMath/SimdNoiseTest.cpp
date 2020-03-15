#include <FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/SimdMath/SimdNoise.h>
#include <Foundation/SimdMath/SimdRandom.h>
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
      sOutFile.Format(":output/SimdNoise/result-perlin_{}.tga", uiNumOctaves);

      EZ_TEST_BOOL(image.SaveTo(sOutFile).Succeeded());

      ezStringBuilder sInFile;
      sInFile.Format("SimdNoise/perlin_{}.tga", uiNumOctaves);
      EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(sInFile), "Noise image file is missing: '%s'", sInFile.GetData());

      EZ_TEST_FILES(sOutFile, sInFile, "");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Random")
  {
    ezUInt32 histogram[256] = {};

    for (ezUInt32 i = 0; i < 10000; ++i)
    {
      ezSimdVec4u seed = ezSimdVec4u(i * 4) + ezSimdVec4u(0, 1, 2, 3);
      ezSimdVec4f randomValues = ezSimdRandom::FloatMinMax(seed, ezSimdVec4f::ZeroVector(), ezSimdVec4f(256.0f));
      ezSimdVec4i randomValuesAsInt = ezSimdVec4i::Truncate(randomValues);

      ++histogram[randomValuesAsInt.x()];
      ++histogram[randomValuesAsInt.y()];
      ++histogram[randomValuesAsInt.z()];
      ++histogram[randomValuesAsInt.w()];
    }

    const char* szOutFile = ":output/SimdNoise/result-random.csv";
    {
      ezFileWriter fileWriter;
      EZ_TEST_BOOL(fileWriter.Open(szOutFile).Succeeded());

      ezStringBuilder sLine;
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(histogram); ++i)
      {
        sLine.Format("{},\n", histogram[i]);
        fileWriter.WriteBytes(sLine.GetData(), sLine.GetElementCount());
      }
    }

    const char* szInFile = "SimdNoise/random.csv";
    EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(szInFile), "Random histogram file is missing: '%s'", szInFile);

    EZ_TEST_TEXT_FILES(szOutFile, szInFile, "");
  }

  ezFileSystem::RemoveDataDirectoryGroup("SimdNoise");
}
