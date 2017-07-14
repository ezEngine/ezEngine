#include "Main.h"
#include <wincodec.h>

/// \todo volume texture creation
/// \todo resizing or downscaling to closest POT or max resolution
/// \todo normalmap generation from heightmaps
/// \todo Normalmap flag (mipmaps?)
/// \todo sRGB auto detection
/// \todo Return loaded image pointer, if no combination is necessary
/// \todo Handle different input sizes
/// \todo Check to generate Mipmaps in Linear space (does that make a difference??)
/// \todo Different mipmap generation for alpha


/// \todo Reading (compressed) TGA very slow
/// \todo Optimize image compositing


/**** Usage ****

-out "file" -> defines where to write the output to
-in "file" -> same as -in0 "file"
-in0 "file" -> defines input file 0
-in31 "file" -> defines input file 31
-mipmap -> enables mipmap generation
-srgb -> the output format will be an SRGB format, otherwise linear, cannot be used for 1 and 2 channel formats
-compress -> the output format will be a compressed format
-channels X -> the output format will have 1, 2, 3 or 4 channels, default is 4
-r inX.r -> the output RED channel is taken from the RED channel of input file X.
-g inX.b -> the output GREEN channel is taken from the BLUE channel of input file X.
-b inX.rgb -> the output BLUE channel is the weighted average of the RGB channels in input file X.
-rgba inX.rrra -> the output RGB cannels are all initialized from the RED channel of input X. Alpha is copied directly.
-cubemap -> The 6 given 2D input textures are supposed to be combined as a cubemap

For the output you can use
-r, -g, -b, -a, -rg, -rgb or -rgba

For input you can use
r,g,b,a
and any combination of rgb, bgar, etc. for swizzling

When a single channel is written (e.g. -r) you can read from multiple channels (e.g. rgb) to create an averaged value.

For cubemaps you can pass in single DDS textures, if they are already packed as a cubemap.
Or you can pass in 6 2D textures and the -cubemap command to combine them.
Instead of -inX you can also use -front, -back, -left, -right, -top, -bottom or -nx, -px, -ny, -py, -nz, -pz to indicate
the individual cubemap faces.
Cubemap channels cannot be combined from multiple input textures.


*/

ezApplication::ApplicationExecution ezTexConv::Run()
{
  ezApplication::ApplicationExecution res = RunInternal();
  if (GetReturnCode() == OK)
  {
    if (m_bHasOutput && m_FileOut.Close().Failed())
    {
      SetReturnCode(TexConvReturnCodes::FAILED_WRITE_OUTPUT);
      ezLog::Error("Could not open output file for writing: '{0}'", m_sOutputFile);
    }
  }
  else
  {
    m_FileOut.Discard();
  }
  return res;
}

ezApplication::ApplicationExecution ezTexConv::RunInternal()
{
  // general failure
  SetReturnCode(TexConvReturnCodes::UNKNOWN_FAILURE);

  PrintConfig();

  if (ValidateConfiguration().Failed())
    return ezApplication::Quit;

  if (!m_sOutputFile.IsEmpty())
  {
    m_FileOut.SetOutput(m_sOutputFile);
    m_bHasOutput = true;
  }

  if (m_TextureType == TextureType::DecalAtlas)
  {
    if (CreateDecalAtlas().Failed())
    {
      ezLog::Error("Failed to create decal atlas texture");
    }

    return ezApplication::Quit;
  }

  if (LoadInputs().Failed())
    return ezApplication::Quit;

  if (CanPassThroughInput())
  {
    ezLog::Info("Input can be passed through");

    if (m_bHasOutput)
    {
      if (PassImageThrough().Failed())
        return ezApplication::Quit;
    }

    if (!m_sThumbnailFile.IsEmpty())
    {
      if (ConvertInputsToRGBA().Failed())
        return ezApplication::Quit;

      const ezImage& img = m_InputImages[0];

      Image srcImg;
      srcImg.width = img.GetWidth();
      srcImg.height = img.GetHeight();
      srcImg.rowPitch = img.GetRowPitch();
      srcImg.slicePitch = img.GetDepthPitch();
      srcImg.format = (DXGI_FORMAT)ezImageFormatMappings::ToDxgiFormat(img.GetImageFormat());
      srcImg.pixels = const_cast<ezUInt8*>(img.GetPixelPointer<ezUInt8>(0, 0));

      m_pCurrentImage = make_shared<ScratchImage>();
      m_pCurrentImage->InitializeFromImage(srcImg);

      SaveThumbnail();
    }
  }
  else
  {
    if (ConvertInputsToRGBA().Failed())
      return ezApplication::Quit;

    if (m_TextureType == TextureType::Cubemap)
    {
      if (CreateTextureCube().Failed())
        return ezApplication::Quit;

      // The resolution might change while converting to a cube map
      // Check the for possible compression after conversion
      CheckCompression();
    }
    else
    {
      ezImage* pCombined = CreateCombined2DImage(m_2dSource);
      if (CreateTexture2D(pCombined, true).Failed())
        return ezApplication::Quit;
    }

    if (GenerateMipmaps().Failed())
      return ezApplication::Quit;

    if (ApplyPremultiplyAlpha().Failed())
      return ezApplication::Quit;

    SaveThumbnail();

    if (m_bHasOutput)
    {
      if (ConvertToOutputFormat().Failed())
        return ezApplication::Quit;

      if (SaveResultToDDS(m_FileOut).Failed())
        return ezApplication::Quit;
    }
  }

  m_pCurrentImage = nullptr;

  // everything is fine
  SetReturnCode(TexConvReturnCodes::OK);

  return ezApplication::Quit;
}



ezResult ezTexConv::SaveThumbnail()
{
  if (m_sThumbnailFile.IsEmpty())
    return EZ_SUCCESS;

  ezLog::Info("Thumbnail: '{0}'", m_sThumbnailFile);

  const ezUInt32 uiThumbnailSize = 128;

  ezUInt32 iThumbnailMip = 0;

  // if we have mipmaps, usually everything is great (because mipmap generation seems to always work correctly)
  // when we do not have mipmaps, everything falls apart and we need to work around lots of problems
  for (ezUInt32 i = 0; i < m_pCurrentImage->GetMetadata().mipLevels; ++i)
  {
    iThumbnailMip = i;

    if (m_pCurrentImage->GetImage(i, 0, 0)->width <= uiThumbnailSize &&
        m_pCurrentImage->GetImage(i, 0, 0)->height <= uiThumbnailSize)
      break;
  }

  shared_ptr<ScratchImage> pThumbnail = make_shared<ScratchImage>();
  {

    DXGI_FORMAT dxgi = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    // enforce sRGB format (and yes, in the non-sRGB case we set the sRGB format and vice versa...)
    if (m_bSRGBOutput)
    {
      dxgi = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    auto pSrcMip = m_pCurrentImage->GetImage(iThumbnailMip, 0, 0);
    auto meta = m_pCurrentImage->GetMetadata();
    meta.mipLevels = 1;
    meta.width = pSrcMip->width;
    meta.height = pSrcMip->height;
    meta.arraySize = 1;
    meta.depth = 1;
    meta.dimension = TEX_DIMENSION_TEXTURE2D;
    meta.miscFlags &= ~TEX_MISC_TEXTURECUBE; // remove the cubemap flag, we only use one face
    meta.miscFlags2 = 0;

    if (FAILED(Convert(pSrcMip, 1, meta, dxgi, TEX_FILTER_DEFAULT, 0.0f, *pThumbnail.get())))
    {
      SetReturnCode(TexConvReturnCodes::FAILED_CONVERT_TO_OUTPUT_FORMAT);
      ezLog::Error("Failed to convert uncompressed float image to R8G8B8A8 for thumbnail");
      return EZ_FAILURE;
    }
  }

  const Image* pThumbnailSrc = pThumbnail->GetImage(0, 0, 0);
  ScratchImage temp;

  if (pThumbnailSrc->width > uiThumbnailSize || pThumbnailSrc->height > uiThumbnailSize)
  {
    ezUInt32 w, h;

    if (pThumbnailSrc->width == pThumbnailSrc->height)
    {
      w = uiThumbnailSize;
      h = uiThumbnailSize;
    }
    else if (pThumbnailSrc->width >= pThumbnailSrc->height)
    {
      const float fAdjust = (float)uiThumbnailSize / (float)pThumbnailSrc->width;
      w = uiThumbnailSize;
      h = (ezUInt32)(pThumbnailSrc->height * fAdjust);
    }
    else
    {
      const float fAdjust = (float)uiThumbnailSize / (float)pThumbnailSrc->height;
      w = (ezUInt32)(pThumbnailSrc->width * fAdjust);
      h = uiThumbnailSize;
    }

    // there are multiple situations in which this call fails :(
    // 1) When a linear format was selected, and thus DXGI_FORMAT_R8G8B8A8_UNORM_SRGB was set above,
    //    we hack around this by overriding the sRGB format during the resize
    // 2) When 'pass through' mode is active and the input texture uses an unsupported format (e.g. a compressed one),
    //    this should be worked around through the conversion above, but not sure that always works

    if (!m_bSRGBOutput)
    {
      // Resize doesn't support SRGB, so claim the image were non-sRGB
      const_cast<Image*>(pThumbnailSrc)->format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    const HRESULT res = Resize(*pThumbnailSrc, w, h, TEX_FILTER_BOX | TEX_FILTER_SEPARATE_ALPHA, temp);
    if (FAILED(res))
    {
      ezLog::Error("Failed to resize image for thumbnail");
      return EZ_FAILURE;
    }

    if (!m_bSRGBOutput)
    {
      // restore the sRGB format
      const_cast<Image*>(pThumbnailSrc)->format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    }

    pThumbnailSrc = temp.GetImage(0, 0, 0);
  }

  if (FAILED(SaveToWICFile(*pThumbnailSrc, 0, GUID_ContainerFormatJpeg, ezStringWChar(m_sThumbnailFile).GetData())))
  {
    ezLog::Error("Failed to save thumbnail to '{0}'", m_sThumbnailFile);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezTexConv::BeforeCoreStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("texconv");
}

const char* ezTexConv::TranslateReturnCode() const
{
  switch (GetReturnCode())
  {
  case TexConvReturnCodes::UNKNOWN_FAILURE:
    return "UNKNOWN_FAILURE";
  case TexConvReturnCodes::VALIDATION_FAILED:
    return "VALIDATION_FAILED";
  case TexConvReturnCodes::BAD_SINGLE_CUBEMAP_FILE:
    return "BAD_SINGLE_CUBEMAP_FILE";
  case TexConvReturnCodes::FAILED_CONVERT_INPUT_TO_RGBA:
    return "FAILED_CONVERT_INPUT_TO_RGBA";
  case TexConvReturnCodes::FAILED_WRITE_OUTPUT:
    return "FAILED_WRITE_OUTPUT";
  case TexConvReturnCodes::FAILED_LOAD_INPUTS:
    return "FAILED_LOAD_INPUTS";
  case TexConvReturnCodes::BAD_INPUT_RESOLUTIONS:
    return "BAD_INPUT_RESOLUTIONS";
  case TexConvReturnCodes::FAILED_PASS_THROUGH:
    return "FAILED_PASS_THROUGH";
  case TexConvReturnCodes::FAILED_MIPMAP_GENERATION:
    return "FAILED_MIPMAP_GENERATION";
  case TexConvReturnCodes::FAILED_BC_COMPRESSION:
    return "FAILED_BC_COMPRESSION";
  case TexConvReturnCodes::FAILED_CONVERT_TO_OUTPUT_FORMAT:
    return "FAILED_CONVERT_TO_OUTPUT_FORMAT";
  case TexConvReturnCodes::FAILED_SAVE_AS_DDS:
    return "FAILED_SAVE_AS_DDS";
  case TexConvReturnCodes::FAILED_INITIALIZE_CUBEMAP:
    return "FAILED_INITIALIZE_CUBEMAP";
  case TexConvReturnCodes::FAILED_COMBINE_CUBEMAP:
    return "FAILED_COMBINE_CUBEMAP";
  case TexConvReturnCodes::FAILED_PREMULTIPLY_ALPHA:
    return "FAILED_PREMULTIPLY_ALPHA";

  default:
    return "UNKNOWN ERROR CODE";
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezTexConv);


