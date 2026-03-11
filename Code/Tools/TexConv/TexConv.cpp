#include <TexConv/TexConvPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <TexConv/TexConv.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/ImageUtils.h>
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
  ezFileSystem::AddDataDirectory("", "App", ":", ezDataDirUsage::AllowWrites).IgnoreResult();

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
  if (sExt == "EZBINTEXTURE2D")
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
  if (sExt == "EZBINTEXTURE3D")
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
  if (sExt == "EZBINTEXTURECUBE")
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
  if (sExt == "EZBINTEXTUREATLAS")
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
  if (sExt == "EZBINIMAGEDATA")
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

ezResult ezTexConv::WriteTexFile(ezStreamWriter& inout_stream, const ezImage& image)
{
  ezAssetFileHeader asset;
  asset.SetFileHashAndVersion(m_Processor.m_Descriptor.m_uiAssetHash, m_Processor.m_Descriptor.m_uiAssetVersion);

  EZ_SUCCEED_OR_RETURN(asset.Write(inout_stream));

  ezTexFormat texFormat;
  texFormat.m_bSRGB = ezImageFormat::IsSrgb(image.GetImageFormat());
  texFormat.m_AddressModeU = m_Processor.m_Descriptor.m_AddressModeU;
  texFormat.m_AddressModeV = m_Processor.m_Descriptor.m_AddressModeV;
  texFormat.m_AddressModeW = m_Processor.m_Descriptor.m_AddressModeW;
  texFormat.m_TextureFilter = m_Processor.m_Descriptor.m_FilterMode;

  texFormat.WriteTextureHeader(inout_stream);

  ezDdsFileFormat ddsWriter;
  if (ddsWriter.WriteImage(inout_stream, image, "dds").Failed())
  {
    ezLog::Error("Failed to write DDS image chunk to ezTex file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::WriteOutputFile(ezStringView sFile, const ezImage& image)
{
  if (sFile.HasExtension("ezBinImageData"))
  {
    ezDeferredFileWriter file;
    file.SetOutput(sFile);

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
    file.SetOutput(sFile);

    EZ_SUCCEED_OR_RETURN(WriteTexFile(file, image));

    return file.Close();
  }
  else
  {
    return image.SaveTo(sFile);
  }
}

ezResult ezTexConv::ReduceSingleFile(ezStringView sInputFile, ezStringView sOutputDir, ezStringView sExplicitOutputFile)
{
  ezImage image;
  if (image.LoadFrom(sInputFile).Failed())
  {
    ezLog::Error("Failed to load input image '{}'.", sInputFile);
    return EZ_FAILURE;
  }

  bool bHasAlpha = false;
  if (ezImageFormat::GetNumChannels(image.GetImageFormat()) >= 4)
  {
    if (image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Succeeded())
    {
      const float* pColors = image.GetPixelPointer<float>();
      pColors += 3; // offset to alpha channel

      EZ_ASSERT_DEV(image.GetRowPitch() == image.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      bool bAllOpaque = true;
      bool bAllTransparent = true;

      for (ezUInt32 i = 0; i < image.GetWidth() * image.GetHeight(); ++i)
      {
        const float a = *pColors;
        bAllOpaque = bAllOpaque && ezMath::IsEqual(a, 1.0f, 1.0f / 255.0f);
        bAllTransparent = bAllTransparent && ezMath::IsEqual(a, 0.0f, 1.0f / 255.0f);
        pColors += 4;

        if (!bAllOpaque && !bAllTransparent)
        {
          bHasAlpha = true;
          break;
        }
      }
    }
    else
    {
      // Conversion failed; assume alpha is present to avoid data loss.
      bHasAlpha = true;
    }
  }

  const ezStringView sExt = bHasAlpha ? "png" : "jpg";

  // Determine output path:
  //   sExplicitOutputFile takes priority (single-file mode with -out as a file path)
  //   sOutputDir places the file in a given folder (folder mode, or single-file with -out as directory)
  //   otherwise output goes next to the input file
  ezStringBuilder sOutputFile;
  if (!sExplicitOutputFile.IsEmpty())
  {
    sOutputFile = sExplicitOutputFile;
  }
  else if (!sOutputDir.IsEmpty())
  {
    sOutputFile = sOutputDir;
    sOutputFile.AppendPath(ezPathUtils::GetFileName(sInputFile));
    sOutputFile.ChangeFileExtension(sExt);
  }
  else
  {
    sOutputFile = sInputFile;
    sOutputFile.ChangeFileExtension(sExt);
  }

  if (ezOSFile::ExistsFile(sOutputFile))
  {
    ezLog::Info("Skipping '{}' (output already exists).", sOutputFile);
    return EZ_SUCCESS;
  }

  if (image.SaveTo(sOutputFile).Failed())
  {
    ezLog::Error("Failed to write output image '{}'.", sOutputFile);
    return EZ_FAILURE;
  }

  ezLog::Success("Wrote '{}' ({})", sOutputFile, bHasAlpha ? "has alpha -> PNG" : "no alpha -> JPG");

  if (m_bDeleteSource)
  {
    if (ezOSFile::DeleteFile(sInputFile).Failed())
    {
      ezLog::Error("Failed to delete source file '{}'.", sInputFile);
      return EZ_FAILURE;
    }

    ezLog::Dev("Deleted source file '{}'.", sInputFile);
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::RunReduce()
{
  ezStringBuilder sInputPath = m_sReduceInputFile;

  // Check for trailing '*' which signals recursive folder processing
  bool bRecursive = false;
  if (sInputPath.EndsWith("*"))
  {
    bRecursive = true;
    sInputPath.Shrink(0, 1); // remove trailing '*'
    sInputPath.Trim("/\\");  // remove any trailing separator left behind
  }

  // Single file mode: -out is either a redirect directory or a direct output file path
  if (ezOSFile::ExistsFile(sInputPath))
  {
    const ezStringView sOutputDir = (!m_sOutputFile.IsEmpty() && ezOSFile::ExistsDirectory(m_sOutputFile)) ? ezStringView(m_sOutputFile) : ezStringView();
    const ezStringView sOutputFile = (!m_sOutputFile.IsEmpty() && !ezOSFile::ExistsDirectory(m_sOutputFile)) ? ezStringView(m_sOutputFile) : ezStringView();
    return ReduceSingleFile(sInputPath, sOutputDir, sOutputFile);
  }

  // Folder mode: parse -out, which may end with '*' to mirror the input subfolder structure
  ezStringBuilder sOutputBase = m_sOutputFile;
  bool bMirrorStructure = false;
  if (sOutputBase.EndsWith("*"))
  {
    bMirrorStructure = true;
    sOutputBase.Shrink(0, 1);
    sOutputBase.Trim("/\\");
  }

  if (!sOutputBase.IsEmpty() && !ezOSFile::ExistsDirectory(sOutputBase))
  {
    ezLog::Error("The -out path '{}' is not an existing directory.", sOutputBase);
    return EZ_FAILURE;
  }

  if (ezOSFile::ExistsDirectory(sInputPath))
  {
    // Recursive iteration does not support wildcards; filter by extension manually instead.
    const ezFileSystemIteratorFlags::Enum flags = bRecursive ? ezFileSystemIteratorFlags::ReportFilesRecursive : ezFileSystemIteratorFlags::ReportFiles;

    ezUInt32 uiConverted = 0;
    ezUInt32 uiFailed = 0;

    bool bAnyFound = false;

    ezStringBuilder sFullPath;
    ezStringBuilder sFileOutputDir;

    {
      ezStringBuilder sSearch = sInputPath;
      if (!bRecursive)
        sSearch.AppendPath("*");

      ezFileSystemIterator iter;
      iter.StartSearch(sSearch, flags);

      for (; iter.IsValid(); iter.Next())
      {
        const ezStringView sName = iter.GetStats().m_sName;
        if (!sName.HasExtension("dds") && !sName.HasExtension("tga"))
          continue;

        bAnyFound = true;

        sFullPath = iter.GetCurrentPath();
        sFullPath.AppendPath(iter.GetStats().m_sName);

        if (sOutputBase.IsEmpty())
        {
          // No -out: place output next to each input file
          sFileOutputDir.Clear();
        }
        else if (bMirrorStructure)
        {
          // -out ends with '*': mirror the subfolder structure under sOutputBase.
          // iter.GetCurrentPath() is the directory containing the current file.
          // Strip the sInputPath prefix to obtain the relative subdirectory.
          ezStringView sCurDir = iter.GetCurrentPath();
          ezStringView sRelativeDir;
          if (sCurDir.StartsWith(sInputPath))
          {
            sRelativeDir = ezStringView(sCurDir.GetStartPointer() + sInputPath.GetElementCount());
            // trim any leading separator
            while (!sRelativeDir.IsEmpty() && (sRelativeDir.GetStartPointer()[0] == '/' || sRelativeDir.GetStartPointer()[0] == '\\'))
            {
              sRelativeDir = ezStringView(sRelativeDir.GetStartPointer() + 1, sRelativeDir.GetEndPointer());
            }
          }
          sFileOutputDir = sOutputBase;
          if (!sRelativeDir.IsEmpty())
          {
            sFileOutputDir.AppendPath(sRelativeDir);
            ezOSFile::CreateDirectoryStructure(sFileOutputDir).IgnoreResult();
          }
        }
        else
        {
          // -out is a plain directory: place all outputs flat in sOutputBase
          sFileOutputDir = sOutputBase;
        }

        if (ReduceSingleFile(sFullPath, sFileOutputDir).Succeeded())
        {
          ++uiConverted;
        }
        else
        {
          ++uiFailed;
        }
      }
    }

    if (!bAnyFound)
    {
      ezLog::Warning("No DDS or TGA files found in '{}'.", sInputPath);
      return EZ_SUCCESS;
    }

    ezLog::Info("Reduce folder '{}': {} processed, {} failed.", sInputPath, uiConverted, uiFailed);
    return uiFailed == 0 ? EZ_SUCCESS : EZ_FAILURE;
  }

  ezLog::Error("Input path '{}' is neither a file nor an existing directory.", sInputPath);
  return EZ_FAILURE;
}

void ezTexConv::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
  {
    QuitApplication();
    return;
  }

  if (m_Mode == ezTexConvMode::Reduce)
  {
    if (RunReduce().Succeeded())
    {
      SetReturnCode(0);
    }

    QuitApplication();
    return;
  }

  if (m_Mode == ezTexConvMode::Compare)
  {
    if (m_Comparer.Compare().Failed())
    {
      QuitApplication();
      return;
    }

    SetReturnCode(0);

    if (m_Comparer.m_bExceededMSE)
    {
      SetReturnCode(m_Comparer.m_OutputMSE);

      if (!m_sOutputFile.IsEmpty())
      {
        ezStringBuilder tmp;

        tmp.Set(m_sOutputFile, "-rgb.png");
        m_Comparer.m_OutputImageDiffRgb.SaveTo(tmp).IgnoreResult();

        tmp.Set(m_sOutputFile, "-alpha.png");
        m_Comparer.m_OutputImageDiffAlpha.SaveTo(tmp).IgnoreResult();

        if (!m_sHtmlTitle.IsEmpty())
        {
          tmp.Set(m_sOutputFile, ".htm");

          ezFileWriter file;
          if (file.Open(tmp).Succeeded())
          {
            ezStringBuilder html;

            ezImageUtils::CreateImageDiffHtml(html, m_sHtmlTitle, m_Comparer.m_ExtractedExpectedRgb, m_Comparer.m_ExtractedExpectedAlpha, m_Comparer.m_ExtractedActualRgb, m_Comparer.m_ExtractedActualAlpha, m_Comparer.m_OutputImageDiffRgb, m_Comparer.m_OutputImageDiffAlpha, m_Comparer.m_OutputMSE, m_Comparer.m_Descriptor.m_MeanSquareErrorThreshold, m_Comparer.m_uiOutputMinDiffRgb, m_Comparer.m_uiOutputMaxDiffRgb, m_Comparer.m_uiOutputMinDiffAlpha, m_Comparer.m_uiOutputMaxDiffAlpha);

            file.WriteBytes(html.GetData(), html.GetElementCount()).AssertSuccess();
          }
        }
      }
    }
  }
  else
  {
    if (m_Processor.Process().Failed())
    {
      QuitApplication();
      return;
    }

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

      QuitApplication();
      return;
    }

    if (!m_sOutputFile.IsEmpty() && m_Processor.m_OutputImage.IsValid())
    {
      if (WriteOutputFile(m_sOutputFile, m_Processor.m_OutputImage).Failed())
      {
        ezLog::Error("Failed to write main result to '{}'", m_sOutputFile);
        QuitApplication();
        return;
      }

      ezLog::Success("Wrote main result to '{}'", m_sOutputFile);
    }

    if (!m_sOutputThumbnailFile.IsEmpty() && m_Processor.m_ThumbnailOutputImage.IsValid())
    {
      if (m_Processor.m_ThumbnailOutputImage.SaveTo(m_sOutputThumbnailFile).Failed())
      {
        ezLog::Error("Failed to write thumbnail result to '{}'", m_sOutputThumbnailFile);
        QuitApplication();
        return;
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
          QuitApplication();
          return;
        }

        ezLog::Success("Wrote low-res result to '{}'", m_sOutputLowResFile);
      }
    }

    SetReturnCode(0);
  }

  QuitApplication();
}

EZ_APPLICATION_ENTRY_POINT(ezTexConv);
