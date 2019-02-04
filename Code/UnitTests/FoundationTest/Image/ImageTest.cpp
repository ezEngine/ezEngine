#include <PCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Image);

class ezLogIgnore : public ezLogInterface
{
  virtual void HandleLogMessage(const ezLoggingEventData& le) override {}
};

EZ_CREATE_SIMPLE_TEST(Image, Image)
{
  ezLogIgnore LogIgnore;

  const ezStringBuilder sReadDir(">sdk/", ezTestFramework::GetInstance()->GetRelTestDataPath());
  const ezStringBuilder sWriteDir = ezTestFramework::GetInstance()->GetAbsOutputPath();

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir, "ImageTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir, "ImageTest", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BMP - Good")
  {
    const char* testImagesGood[] = {
      "BMPTestImages/good/pal1", "BMPTestImages/good/pal1bg", "BMPTestImages/good/pal1wb", "BMPTestImages/good/pal4",
      "BMPTestImages/good/pal4rle", "BMPTestImages/good/pal8", "BMPTestImages/good/pal8-0", "BMPTestImages/good/pal8nonsquare",
      /*"BMPTestImages/good/pal8os2",*/ "BMPTestImages/good/pal8rle",
      /*"BMPTestImages/good/pal8topdown",*/ "BMPTestImages/good/pal8v4", "BMPTestImages/good/pal8v5", "BMPTestImages/good/pal8w124",
      "BMPTestImages/good/pal8w125", "BMPTestImages/good/pal8w126", "BMPTestImages/good/rgb16", "BMPTestImages/good/rgb16-565pal",
      "BMPTestImages/good/rgb24", "BMPTestImages/good/rgb24pal", "BMPTestImages/good/rgb32", /*"BMPTestImages/good/rgb32bf"*/
    };

    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesGood); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("{0}.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName) == EZ_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        ezStringBuilder fileName;
        fileName.Format(":output/{0}_out.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BMP - Bad")
  {
    const char* testImagesBad[] = {"BMPTestImages/bad/badbitcount", "BMPTestImages/bad/badbitssize",
      /*"BMPTestImages/bad/baddens1", "BMPTestImages/bad/baddens2", "BMPTestImages/bad/badfilesize", "BMPTestImages/bad/badheadersize",*/
      "BMPTestImages/bad/badpalettesize",
      /*"BMPTestImages/bad/badplanes",*/ "BMPTestImages/bad/badrle", "BMPTestImages/bad/badwidth",
      /*"BMPTestImages/bad/pal2",*/ "BMPTestImages/bad/pal8badindex", "BMPTestImages/bad/reallybig", "BMPTestImages/bad/rletopdown",
      "BMPTestImages/bad/shortfile"};


    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesBad); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("{0}.bmp", testImagesBad[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "File does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName, &LogIgnore) == EZ_FAILURE, "Reading image should have failed: '%s'", fileName.GetData());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TGA")
  {
    const char* testImagesGood[] = {
      "TGATestImages/good/RGB", "TGATestImages/good/RGBA", "TGATestImages/good/RGB_RLE", "TGATestImages/good/RGBA_RLE"};

    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesGood); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("{0}.tga", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName) == EZ_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        ezStringBuilder fileName;
        fileName.Format(":output/{0}_out.bmp", testImagesGood[i]);

        ezStringBuilder fileNameExpected;
        fileNameExpected.Format("{0}_expected.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        EZ_TEST_FILES(fileName, fileNameExpected, "");
      }

      {
        ezStringBuilder fileName;
        fileName.Format(":output/{0}_out.tga", testImagesGood[i]);

        ezStringBuilder fileNameExpected;
        fileNameExpected.Format("{0}_expected.tga", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        EZ_TEST_FILES(fileName, fileNameExpected, "");
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Write Image Formats")
  {
    struct ImgTest
    {
      const char* szImage;
      const char* szFormat;
      ezUInt32 uiMSE;
    };

    ImgTest imgTests[] = {
      {"RGB", "tga", 0},
      {"RGBA", "tga", 0},
      {"RGB", "png", 0},
      {"RGBA", "png", 0},
      {"RGB", "jpg", 4650},
      {"RGBA", "jpeg", 16670},
    };

    const char* szTestImagePath = "TGATestImages/good";

    for (int idx = 0; idx < EZ_ARRAY_SIZE(imgTests); ++idx)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("{}/{}.tga", szTestImagePath, imgTests[idx].szImage);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Image file does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName) == EZ_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        ezStringBuilder fileName;
        fileName.Format(":output/WriteImageTest/{}.{}", imgTests[idx].szImage, imgTests[idx].szFormat);

        ezFileSystem::DeleteFile(fileName);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName), "Output image file is missing: '%s'", fileName.GetData());

        ezImage image2;
        EZ_TEST_BOOL_MSG(image2.LoadFrom(fileName).Succeeded(), "Reading written image fialed: '%s'", fileName.GetData());

        image.Convert(ezImageFormat::R8G8B8A8_UNORM_SRGB);
        image2.Convert(ezImageFormat::R8G8B8A8_UNORM_SRGB);

        ezImage diff;
        ezImageUtils::ComputeImageDifferenceABS(image, image2, diff);

        const ezUInt32 uiMSE = ezImageUtils::ComputeMeanSquareError(diff, 32);

        EZ_TEST_BOOL_MSG(uiMSE <= imgTests[idx].uiMSE, "MSE %u is larger than %u for image '%s'", uiMSE, imgTests[idx].uiMSE, fileName.GetData());
      }
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
