#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>
#include <TexConv/DirectXTex/DirectXTex.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Image/Formats/ImageFormatMappings.h>

using namespace DirectX;

class ezTexConv : public ezApplication
{
public:
  ezTexConv()
  {
  }

  virtual void AfterCoreStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("");
  }

  virtual void BeforeCoreShutdown()
  {
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual ezApplication::ApplicationExecution Run() override
  {
    CoInitialize(nullptr);

    ezImage source;
    if (source.LoadFrom("D:/Test.tga").Failed())
      return ezApplication::Quit;

    if (ezImageConversion::Convert(source, source, ezImageFormat::R8G8B8A8_UNORM).Failed())
      return ezApplication::Quit;

    Image srcImg;
    srcImg.width = source.GetWidth();
    srcImg.height = source.GetHeight();
    srcImg.rowPitch = source.GetRowPitch();
    srcImg.slicePitch = source.GetDepthPitch();
    srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(source.GetImageFormat());
    srcImg.pixels = source.GetDataPointer<ezUInt8>();

    //TexMetadata meta;
    ScratchImage img, mip, comp;
    //if (SUCCEEDED(LoadFromTGAFile(L"D:/Test.tga", &meta, img)))
    {
      if (SUCCEEDED(GenerateMipMaps(srcImg, TEX_FILTER_DEFAULT, 0, mip)))
      {
        if (SUCCEEDED(Compress(mip.GetImages(), mip.GetImageCount(), mip.GetMetadata(), DXGI_FORMAT_BC1_UNORM_SRGB, TEX_COMPRESS_DEFAULT, 1.0f, comp)))
        {

          SaveToDDSFile(comp.GetImages(), comp.GetImageCount(), comp.GetMetadata(), 0, L"D:/Output.dds");
        }
      }
    }

    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv);
