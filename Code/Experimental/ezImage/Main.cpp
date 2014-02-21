#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Basics/Types.h>
#include <Foundation/Containers/DynamicArray.h>

#include "Image.h"
#include "Foundation/Configuration/Startup.h"
#include "ImageConversion.h"
#include "ImageConversionMixin.h"
#include "Foundation/IO/FileSystem/FileSystem.h"
#include "BMP/BmpFileFormat.h"
#include "Foundation/IO/FileSystem/FileReader.h"
#include "Foundation/IO/FileSystem/FileWriter.h"
#include "Foundation/IO/FileSystem/DataDirTypeFolder.h"
#include "DDS/DdsFileFormat.h"
#include "Foundation/Logging/HTMLWriter.h"
#include "Foundation/Logging/Log.h"
#include "Foundation/Logging/VisualStudioWriter.h"

int main()
{
  ezStartup::StartupCore();

  ezStringBuilder dataDir = BUILDSYSTEM_OUTPUT_FOLDER;

  dataDir.AppendPath("../../Code/Experimental/ezImage/bmpsuite-2.2");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(dataDir.GetData());

  ezImageFileFormatBase* bmpFormat = new ezBmpFileFormat;

  const char* testImagesGood[] =
  {
    "g/pal1", "g/pal1bg", "g/pal1wb", "g/pal4", "g/pal4rle", "g/pal8", "g/pal8-0", "g/pal8nonsquare",
    /*"g/pal8os2",*/ "g/pal8rle", /*"g/pal8topdown",*/ "g/pal8v4", "g/pal8v5", "g/pal8w124", "g/pal8w125",
    "g/pal8w126", "g/rgb16", "g/rgb16-565pal", "g/rgb24", "g/rgb24pal", "g/rgb32", /*"g/rgb32bf"*/
  };

  for(int i = 0; i < sizeof(testImagesGood) / sizeof(testImagesGood[0]); i++)
  {
    ezImage image;
    {
      ezStringBuilder fileName;
      fileName.Format("%s.bmp", testImagesGood[i]);

      ezResult result = image.LoadFrom(fileName.GetData());

      EZ_ASSERT(result == EZ_SUCCESS, "Reading image failed");
    }

    {
      ezStringBuilder fileName;
      fileName.Format("%s_out.bmp", testImagesGood[i]);

      ezResult result = image.SaveTo(fileName.GetData());

      EZ_ASSERT(result == EZ_SUCCESS, "Writing image failed");
    }
   }

  const char* testImagesBad[] =
  {
    "b/badbitcount", "b/badbitssize", /*"b/baddens1", "b/baddens2", "b/badfilesize", "b/badheadersize",*/ "b/badpalettesize",
    /*"b/badplanes",*/ "b/badrle", "b/badwidth", "b/pal2", "b/pal8badindex", "b/reallybig", "b/rletopdown", "b/shortfile"
  };


  for(int i = 0; i < sizeof(testImagesBad) / sizeof(testImagesBad[0]); i++)
  {
    ezImage image;
    {
      ezStringBuilder fileName;
      fileName.Format("%s.bmp", testImagesBad[i]);

      EZ_ASSERT(ezFileSystem::ExistsFile(fileName.GetData()), "File does not exist");

      ezResult result = image.LoadFrom(fileName.GetData());

      EZ_ASSERT(result == EZ_FAILURE, "Reading image should have failed");
    }
  }

  ezStartup::ShutdownBase();
}
