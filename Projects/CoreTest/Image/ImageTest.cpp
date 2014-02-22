#include <PCH.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/Conversions/ImageConversion.h>
#include <CoreUtils/Image/Formats/ImageFileFormat.h>
#include <CoreUtils/Image/Formats/BmpFileFormat.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Image);

class ezLogIgnore : public ezLogInterface
{
  virtual void HandleLogMessage(const ezLoggingEventData& le) EZ_OVERRIDE
  {
  }
};

EZ_CREATE_SIMPLE_TEST(Image, Image)
{
  ezLogIgnore LogIgnore;

  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/UnitTests/CoreTest/BMPTestImages");

  ezStringBuilder sWriteDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sWriteDir.AppendPath("CoreTest/BMPTestImages");

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::DataDirUsage::ReadOnly, "ImageTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::DataDirUsage::AllowWrites, "ImageTest") == EZ_SUCCESS);

  ezImageFileFormatBase* bmpFormat = new ezBmpFileFormat;

  const char* testImagesGood[] =
  {
    "good/pal1", "good/pal1bg", "good/pal1wb", "good/pal4", "good/pal4rle", "good/pal8", "good/pal8-0", "good/pal8nonsquare",
    /*"good/pal8os2",*/ "good/pal8rle", /*"good/pal8topdown",*/ "good/pal8v4", "good/pal8v5", "good/pal8w124", "good/pal8w125",
    "good/pal8w126", "good/rgb16", "good/rgb16-565pal", "good/rgb24", "good/rgb24pal", "good/rgb32", /*"good/rgb32bf"*/
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

  const char* testImagesBad[] =
  {
    "bad/badbitcount", "bad/badbitssize", /*"bad/baddens1", "bad/baddens2", "bad/badfilesize", "bad/badheadersize",*/ "bad/badpalettesize",
    /*"bad/badplanes",*/ "bad/badrle", "bad/badwidth", /*"bad/pal2",*/ "bad/pal8badindex", "bad/reallybig", "bad/rletopdown", "bad/shortfile"
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

  ezFileSystem::RemoveDataDirectoryGroup("ImageTest");
}
