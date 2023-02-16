#include <TexConv/TexConvPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

ezTexConv::ezTexConv()
  : ezApplication("TexConv")
{
}

ezResult ezTexConv::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void ezTexConv::AfterCoreSystemsStartup()
{
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

void ezTexConv::BeforeCoreSystemsShutdown()
{
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

ezResult ezTexConv::DetectOutputFormat()
{
  if (m_sOutputFile.IsEmpty())
  {
    m_Processor.m_Descriptor.m_OutputType = ezTexConvOutputType::None;
    return EZ_SUCCESS;
  }

  ezStringBuilder sExt = ezPathUtils::GetFileExtension(m_sOutputFile);
  sExt.ToUpper();

  if (sExt == "DDS")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "TGA" || sExt == "PNG")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURE2D")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURE3D")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = true;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTURECUBE")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = true;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZTEXTUREATLAS")
  {
    m_bOutputSupports2D = false;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = true;
    m_bOutputSupportsMipmaps = true;
    m_bOutputSupportsFiltering = true;
    m_bOutputSupportsCompression = true;
    return EZ_SUCCESS;
  }
  if (sExt == "EZIMAGEDATA")
  {
    m_bOutputSupports2D = true;
    m_bOutputSupports3D = false;
    m_bOutputSupportsCube = false;
    m_bOutputSupportsAtlas = false;
    m_bOutputSupportsMipmaps = false;
    m_bOutputSupportsFiltering = false;
    m_bOutputSupportsCompression = false;
    return EZ_SUCCESS;
  }

  ezLog::Error("Output file uses unsupported file format '{}'", sExt);
  return EZ_FAILURE;
}

bool ezTexConv::IsTexFormat() const
{
  const ezStringView ext = ezPathUtils::GetFileExtension(m_sOutputFile);

  return ext.StartsWith_NoCase("ez");
}

ezResult ezTexConv::WriteTexFile(ezStreamWriter& stream, const ezImage& image)
{
  ezAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  EZ_SUCCEED_OR_RETURN(asset.Write(stream));

  ezTexFormat texFormat;
  texFormat.m_bSRGB = ezImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(stream);

  ezDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(stream, image, "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image chunk to ezTex file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::WriteOutputFile(const char* szFile, const ezImage& image)
{
  if (ezPathUtils::HasExtension(szFile, "ezImageData"))
  {
    ezDeferredFileWriter file;
    file.SetOutput(szFile);

    ezAssetFileHeader asset;
    asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    if (asset.Write(file).Failed())
    {
      ezLog::Error("Failed to write asset header to file.");
      return EZ_FAILURE;
    }

    ezUInt8 uiVersion = 1;
    file << uiVersion;

    ezUInt8 uiFormat = 1; // 1 == PNG
    file << uiFormat;

    ezStbImageFileFormats pngWriter;
    if (pngWriter.WriteImage(file, image, "png").Failed())
    {
      ezLog::Error("Failed to write data as PNG to ezImageData file.");
      return EZ_FAILURE;
    }

    return file.Close();
  }
  else if (IsTexFormat())
  {
    ezDeferredFileWriter file;
    file.SetOutput(szFile);

    EZ_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(szFile);
  }
}

ezApplication::Execution ezTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
    return ezApplication::Execution::Quit;

  if (m_Processor.Process().Failed())
    return ezApplication::Execution::Quit;

  if (m_Processor.m_Descriptor.m_OutputType == ezTexConvOutputType::Atlas)
  {
    ezDeferredFileWriter file;
    file.SetOutput(m_sOutputFile);

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

    header.Write(file).IgnoreResult();

    m_Processor.m_TextureAtlas.CopyToStream(file).IgnoreResult();

    if (file.Close().Succeeded())
    {
      SetReturnCode(0);
    }
    else
    {
      ezLog::Error("Failed to write atlas output image.");
    }

    return ezApplication::Execution::Quit;
  }

  if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
  {
    if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
    {
      ezLog::Error("Failed to write main result to '{}'", m_sOutputFile);
      return ezApplication::Execution::Quit;
    }

    ezLog::Success("Wrote main result to '{}'", m_sOutputFile);
  }

  if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
  {
    if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
    {
      ezLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
      return ezApplication::Execution::Quit;
    }

    ezLog::Success("Wrote thumbnail to '{}'", m_sOutputThumbnailFile);
  }

  if (!m_sOutputLowResFile.IsEmpty())
  {
    // the image may not exist, if we do not have enough mips, so make sure any old low-res file is cleaned up
    ezOSFile::DeleteFile(m_sOutputLowResFile).IgnoreResult();

    if (m_Processor.m_LowResOutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputLowResFile, m_Processor.m_LowResOutputImage).Failed())
      {
        ezLog::Error("Failed to write low-res result to '{}'", m_sOutputLowResFile);
        return ezApplication::Execution::Quit;
      }

      ezLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
    }
  }

  SetReturnCode(0);
  return ezApplication::Execution::Quit;
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv);
