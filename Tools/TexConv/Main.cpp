#include "Main.h"

/// \todo volume texture creation
/// \todo resizing or downscaling to closest POT or max resolution
/// \todo normalmap generation from heightmaps
/// \todo Normalmap flag (mipmaps?)
/// \todo sRGB auto detection
/// \todo Return loaded image pointer, if no combination is necessary
/// \todo Handle different input sizes
/// \todo Check to generate Mipmaps in Linear space (does that make a difference??)

/// \todo Write thumbnail to additional location
/// \todo Reading (compressed) TGA very slow
/// \todo cubemap creation
/// \todo Use checked in TexConv (release build) for asset transform
/// \todo Optimize image compositing
/// \todo Use BC1 over BC3, if alpha is all 0 or 1


/**** Usage ****

-out "file" -> defines where to write the output to
-in "file" -> same as -in0 "file"
-in0 "file" -> defines input file 0
-in31 "file" -> defines input file 31
-mipmap -> enables mipmap generation
-srgb -> the output format will be an SRGB format, otherwise linear, cannot be used for 1 and 2 channel formats
-compress -> the output format will be a compressed format
-channels X -> the output format will have 1, 2, 3 or 4 channels, default is 4
-r inX.r -> the output RED channel is taken from the RED channel of input file X. The input channel is considered to be in Gamma space!
-g inX.x -> the output GREEN channel is taken from the RED channel of input file X. The input channel is considered to be in Linear space!
-b inX.rgb -> the output BLUE channel is the weighted average of the RGB channels in input file X. The RGB channels are considered to be in Gamma space!
-rgba inX.rrra -> the output RGB cannels are all initialized from the RED channel of input X. Alpha is copied directly.

For the output you can use
-r, -g, -b, -a, -rg, -rgb, -rgba

For input you can use
r,g,b,a for gamma space values (alpha is always linear, though)
x,y,z,w for linear space values
and any combination of rgb, bgar, xyz, rgxy, etc. for swizzling

When a single channel is written (e.g. -r) you can read from multiple channels (e.g. rgb) to create an averaged value.

*/

ezApplication::ApplicationExecution ezTexConv::Run()
{
  // general failure
  SetReturnCode(1);

  PrintConfig();

  if (ValidateConfiguration().Failed())
  {
    SetReturnCode(2);
    return ezApplication::Quit;
  }

  ezFileWriter fileOut;
  if (fileOut.Open(m_sOutputFile, 1024 * 1024 * 8).Failed())
  {
    SetReturnCode(3);
    ezLog::Error("Could not open output file for writing: '%s'", m_sOutputFile.GetData());
    return ezApplication::Quit;
  }

  if (LoadInputs().Failed())
  {
    SetReturnCode(4);
    return ezApplication::Quit;
  }

  CoInitialize(nullptr);

  if (CanPassThroughInput())
  {
    ezLog::Info("Input can be passed through");

    WriteTexHeader(fileOut);

    ezImage* pImg = &m_InputImages[0];

    ezDdsFileFormat writer;
    if (writer.WriteImage(fileOut, *pImg, ezGlobalLog::GetOrCreateInstance()).Failed())
    {
      SetReturnCode(10);
      return ezApplication::Quit;
    }
  }
  else
  {
    if (ConvertInputsToRGBA().Failed())
    {
      SetReturnCode(4);
      return ezApplication::Quit;
    }

    ezImage* pCombined = CreateCombinedFile(m_2dSource);


    Image srcImg;
    srcImg.width = pCombined->GetWidth();
    srcImg.height = pCombined->GetHeight();
    srcImg.rowPitch = pCombined->GetRowPitch();
    srcImg.slicePitch = pCombined->GetDepthPitch();
    srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(pCombined->GetImageFormat());
    srcImg.pixels = pCombined->GetDataPointer<ezUInt8>();

    ScratchImage mip, comp, channel;
    ScratchImage* pCurScratch = nullptr;

    if (m_bGeneratedMipmaps)
    {
      if (FAILED(GenerateMipMaps(srcImg, TEX_FILTER_DEFAULT, 0, mip)))
      {
        SetReturnCode(5);
        ezLog::Error("Mipmap generation failed");
        return ezApplication::Quit;
      }

      pCurScratch = &mip;
    }

    const ezImageFormat::Enum outputFormat = ChooseOutputFormat(false /*m_bSRGBOutput*/); // we don't want the implict sRGB conversion of MS TexConv, so just write to non-sRGB target
    const DXGI_FORMAT dxgi = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(outputFormat);

    Blob outputBlob;

    if (m_bCompress)
    {
      if (pCurScratch != nullptr)
      {
        if (FAILED(Compress(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), dxgi, TEX_COMPRESS_DEFAULT, 1.0f, comp)))
        {
          SetReturnCode(6);
          ezLog::Error("Block compression failed");
          return ezApplication::Quit;
        }
      }
      else
      {
        if (FAILED(Compress(srcImg, dxgi, TEX_COMPRESS_DEFAULT, 1.0f, comp)))
        {
          SetReturnCode(6);
          ezLog::Error("Block compression failed");
          return ezApplication::Quit;
        }
      }

      if (FAILED(SaveToDDSMemory(comp.GetImages(), comp.GetImageCount(), comp.GetMetadata(), 0, outputBlob)))
      {
        SetReturnCode(7);
        ezLog::Error("Failed to write compressed image to file '%s'", m_sOutputFile.GetData());
        return ezApplication::Quit;
      }
    }
    else
    {
      if (outputFormat != pCombined->GetImageFormat())
      {
        if (pCurScratch != nullptr)
        {
          if (FAILED(Convert(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), dxgi, TEX_FILTER_DEFAULT, 0.0f, channel)))
          {
            SetReturnCode(8);
            ezLog::Error("Failed to convert uncompressed image to %u channels", m_uiOutputChannels);
            return ezApplication::Quit;
          }
        }
        else
        {
          if (FAILED(Convert(srcImg, dxgi, TEX_FILTER_DEFAULT, 0.0f, channel)))
          {
            SetReturnCode(8);
            ezLog::Error("Failed to convert uncompressed image to %u channels", m_uiOutputChannels);
            return ezApplication::Quit;
          }
        }

        pCurScratch = &channel;
      }

      if (pCurScratch != nullptr)
      {
        if (FAILED(SaveToDDSMemory(pCurScratch->GetImages(), pCurScratch->GetImageCount(), pCurScratch->GetMetadata(), 0, outputBlob)))
        {
          SetReturnCode(9);
          ezLog::Error("Failed to write uncompressed image to file '%s'", m_sOutputFile.GetData());
          return ezApplication::Quit;
        }
      }
      else
      {
        if (FAILED(SaveToDDSMemory(srcImg, 0, outputBlob)))
        {
          SetReturnCode(9);
          ezLog::Error("Failed to write uncompressed image to file '%s'", m_sOutputFile.GetData());
          return ezApplication::Quit;
        }
      }
    }

    WriteTexHeader(fileOut);

    fileOut.WriteBytes(outputBlob.GetBufferPointer(), outputBlob.GetBufferSize());
  }

  // everything is fine
  SetReturnCode(0);
  return ezApplication::Quit;
}



EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv);
