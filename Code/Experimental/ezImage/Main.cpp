#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Basics/Types.h>
#include <Foundation/Containers/DynamicArray.h>

#include "ImageDefinitions.h"
#include "Image.h"
#include "Foundation/Configuration/Startup.h"
#include "ImageConversion.h"
#include "ImageConversionMixin.h"
#include "Foundation/IO/FileSystem/FileSystem.h"
#include "BMP/BmpFileFormat.h"
#include "Foundation/IO/FileSystem/FileReader.h"
#include "Foundation/IO/FileSystem/FileWriter.h"
#include "Foundation/IO/FileSystem/DataDirTypeFolder.h"

int main()
{
  ezStartup::StartupCore();
  // Does this now work by default?
  // ezImageConversion::Startup();
  // ezImageFormat::Initialize();

  ezStringBuilder dataDir = BUILDSYSTEM_OUTPUT_FOLDER;

  dataDir.AppendPath("../../Code/Experimental/ezImage/bmpsuite-2.2");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(dataDir.GetData());

  ezIImageFileFormat* bmpFormat = new ezBmpFileFormat;

  const char* testImages[] =
  {
    "g/pal1", "g/pal1bg", "g/pal1wb", "g/pal4", "g/pal4rle", "g/pal8", "g/pal8-0", "g/pal8nonsquare",
    /*"g/pal8os2",*/ "g/pal8rle", /*"g/pal8topdown",*/ "g/pal8v4", "g/pal8v5", "g/pal8w124", "g/pal8w125",
    "g/pal8w126", "g/rgb16", "g/rgb16-565pal", "g/rgb24", "g/rgb24pal", "g/rgb32", /*"g/rgb32bf"*/
  };

  for(int i = 0; i < sizeof(testImages) / sizeof(testImages[0]); i++)
  {
    ezImage image;
    {
      ezStringBuilder fileName;
      fileName.Format("%s.bmp", testImages[i]);

      ezFileReader reader;
      ezResult result = reader.Open(fileName.GetData());

      EZ_ASSERT(result == EZ_SUCCESS, "Reading image failed");

      ezStringBuilder errorString;
      result = bmpFormat->ReadImage(reader, image, errorString);

      EZ_ASSERT(result == EZ_SUCCESS, "Reading image failed: %s", errorString.GetData());
    }

    {
      ezStringBuilder fileName;
      fileName.Format("%s_out.bmp", testImages[i]);

      ezFileWriter writer;
      writer.Open(fileName.GetData());

      ezStringBuilder errorString;
      ezResult result = bmpFormat->WriteImage(writer, image, errorString);

      EZ_ASSERT(result == EZ_SUCCESS, "Reading image failed: %s", errorString.GetData());
    }
   }

  delete bmpFormat;

  // Does this now work by default?
  // ezImageConversion::Shutdown();
  ezStartup::ShutdownBase();
  ezStartup::ShutdownCore();
}