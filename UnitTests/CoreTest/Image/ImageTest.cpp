#include <PCH.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Image/Formats/ImageFileFormat.h>
#include <CoreUtils/Image/Formats/BmpFileFormat.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Image);

class ezLogIgnore : public ezLogInterface
{
  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
  }
};

EZ_CREATE_SIMPLE_TEST(Image, Image)
{
  ezLogIgnore LogIgnore;

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/UnitTests/CoreTest");

  ezStringBuilder sWriteDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sWriteDir.AppendPath("CoreTest");

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "ImageTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::AllowWrites, "ImageTest") == EZ_SUCCESS);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BMP - Good")
  {
    const char* testImagesGood[] =
    {
      "BMPTestImages/good/pal1", "BMPTestImages/good/pal1bg", "BMPTestImages/good/pal1wb", "BMPTestImages/good/pal4", "BMPTestImages/good/pal4rle", "BMPTestImages/good/pal8", "BMPTestImages/good/pal8-0", "BMPTestImages/good/pal8nonsquare",
      /*"BMPTestImages/good/pal8os2",*/ "BMPTestImages/good/pal8rle", /*"BMPTestImages/good/pal8topdown",*/ "BMPTestImages/good/pal8v4", "BMPTestImages/good/pal8v5", "BMPTestImages/good/pal8w124", "BMPTestImages/good/pal8w125",
      "BMPTestImages/good/pal8w126", "BMPTestImages/good/rgb16", "BMPTestImages/good/rgb16-565pal", "BMPTestImages/good/rgb24", "BMPTestImages/good/rgb24pal", "BMPTestImages/good/rgb32", /*"BMPTestImages/good/rgb32bf"*/
    };

    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesGood); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("%s.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "Image file does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName.GetData()) == EZ_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        ezStringBuilder fileName;
        fileName.Format("%s_out.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName.GetData()) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "Output image file is missing: '%s'", fileName.GetData());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BMP - Bad")
  {
    const char* testImagesBad[] =
    {
      "BMPTestImages/bad/badbitcount", "BMPTestImages/bad/badbitssize", /*"BMPTestImages/bad/baddens1", "BMPTestImages/bad/baddens2", "BMPTestImages/bad/badfilesize", "BMPTestImages/bad/badheadersize",*/ "BMPTestImages/bad/badpalettesize",
      /*"BMPTestImages/bad/badplanes",*/ "BMPTestImages/bad/badrle", "BMPTestImages/bad/badwidth", /*"BMPTestImages/bad/pal2",*/ "BMPTestImages/bad/pal8badindex", "BMPTestImages/bad/reallybig", "BMPTestImages/bad/rletopdown", "BMPTestImages/bad/shortfile"
    };


    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesBad); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("%s.bmp", testImagesBad[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "File does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName.GetData(), &LogIgnore) == EZ_FAILURE, "Reading image should have failed: '%s'", fileName.GetData());
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TGA")
  {
    const char* testImagesGood[] =
    {
      "TGATestImages/good/RGB", "TGATestImages/good/RGBA", "TGATestImages/good/RGB_RLE", "TGATestImages/good/RGBA_RLE"
    };

    for (int i = 0; i < EZ_ARRAY_SIZE(testImagesGood); i++)
    {
      ezImage image;
      {
        ezStringBuilder fileName;
        fileName.Format("%s.tga", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "Image file does not exist: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(image.LoadFrom(fileName.GetData()) == EZ_SUCCESS, "Reading image failed: '%s'", fileName.GetData());
      }

      {
        ezStringBuilder fileName;
        fileName.Format("%s_out.bmp", testImagesGood[i]);

        ezStringBuilder fileNameExpected;
        fileNameExpected.Format("%s_expected.bmp", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName.GetData()) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "Output image file is missing: '%s'", fileName.GetData());

        EZ_TEST_FILES(fileName.GetData(), fileNameExpected.GetData(), "");
      }

      {
        ezStringBuilder fileName;
        fileName.Format("%s_out.tga", testImagesGood[i]);

        ezStringBuilder fileNameExpected;
        fileNameExpected.Format("%s_expected.tga", testImagesGood[i]);

        EZ_TEST_BOOL_MSG(image.SaveTo(fileName.GetData()) == EZ_SUCCESS, "Writing image failed: '%s'", fileName.GetData());
        EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "Output image file is missing: '%s'", fileName.GetData());

        EZ_TEST_FILES(fileName.GetData(), fileNameExpected.GetData(), "");
      }
    }
  }

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
