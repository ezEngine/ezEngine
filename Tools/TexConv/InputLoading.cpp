#include "Main.h"


ezResult ezTexConv::LoadSingleInputFile(const char* szFile)
{
  ezImage& source = m_InputImages.ExpandAndGetRef();
  if (source.LoadFrom(szFile).Failed())
  {
    ezLog::Error("Failed to load file '{0}'", szFile);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::LoadInputs()
{
  EZ_LOG_BLOCK("Load Inputs");
  m_InputImages.Reserve(m_InputFileNames.GetCount());

  for (const auto& in : m_InputFileNames)
  {
    if (LoadSingleInputFile(in).Failed())
    {
      SetReturnCode(TexConvReturnCodes::FAILED_LOAD_INPUTS);
      return EZ_FAILURE;
    }
  }


  // Some evaluation.
  if (m_TextureType != TextureType::Cubemap)
  {
    // The resolution might change while converting to a cube map
    // Check the for possible compression after conversion
    CheckCompression();
  }

  // Check for same resolutions.
  for (ezUInt32 i = 1; i < m_InputImages.GetCount(); ++i)
  {
    if (m_InputImages[i].GetWidth() != m_InputImages[0].GetWidth() || m_InputImages[i].GetHeight() != m_InputImages[0].GetHeight())
    {
      SetReturnCode(TexConvReturnCodes::BAD_INPUT_RESOLUTIONS);
      ezLog::Error("Input image {0} has a different resolution than image 0. This is currently not supported.", i);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

void ezTexConv::CheckCompression()
{
  // Check if compression is possible.
  if (m_bCompress)
  {
    for (ezUInt32 i = 0; i < m_InputImages.GetCount(); ++i)
    {
      // We're doing block compression in any case, so we can only support width/height beeing a multiple of 4.
      // Since this must apply to all mip maps this means effectively that we need a 2^n texture.
      // Give out an error and disable compression if this is not the case.
      if (!ezMath::IsPowerOf(m_InputImages[i].GetWidth(), 2) || !ezMath::IsPowerOf(m_InputImages[i].GetHeight(), 2))
      {
        ezLog::Error("Input image '{0}' cannot be compressed since it's height/width is not a power of 2.", m_InputFileNames[i]);
        m_bCompress = false;
        break;
      }
    }
  }
}

ezResult ezTexConv::ConvertInputsToRGBAf32()
{
  for (ezUInt32 i = 0; i < m_InputImages.GetCount(); ++i)
  {
    if (ezImageConversion::Convert(m_InputImages[i], m_InputImages[i], ezImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      SetReturnCode(TexConvReturnCodes::FAILED_CONVERT_INPUT_TO_RGBA);
      ezLog::Error("Failed to convert input {0} from format {1} to R32G32B32A32_FLOAT. Format is not supported.", i,
                   ezImageFormat::GetName(m_InputImages[i].GetImageFormat()));
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConv::ClampToMaxResolution()
{
  // scaling down cubemaps is currently not supported
  if (m_TextureType != TextureType::Texture2D)
    return EZ_SUCCESS;

  // the the input image is already small enough, don't do anything
  if (m_pCurrentImage->GetMetadata().width <= m_uiMaxResolution && m_pCurrentImage->GetMetadata().height <= m_uiMaxResolution &&
      m_pCurrentImage->GetMetadata().depth <= m_uiMaxResolution)
    return EZ_SUCCESS;

  ezUInt32 iTargetMip = 0;

  // if we have mipmaps, find the mip that is closest to the target resolution
  for (ezUInt32 i = 0; i < m_pCurrentImage->GetMetadata().mipLevels; ++i)
  {
    iTargetMip = i;

    if (m_pCurrentImage->GetImage(i, 0, 0)->width <= m_uiMaxResolution && m_pCurrentImage->GetImage(i, 0, 0)->height <= m_uiMaxResolution)
      break;
  }

  const Image* pLowerResSrc = m_pCurrentImage->GetImage(iTargetMip, 0, 0);
  ScratchImage temp;

  // scale down, if the found mipmap is not small enough
  if (pLowerResSrc->width > m_uiMaxResolution || pLowerResSrc->height > m_uiMaxResolution)
  {
    ezUInt32 w, h;

    // keep aspect ratio
    if (pLowerResSrc->width == pLowerResSrc->height)
    {
      w = m_uiMaxResolution;
      h = m_uiMaxResolution;
    }
    else if (pLowerResSrc->width >= pLowerResSrc->height)
    {
      const float fAdjust = (float)m_uiMaxResolution / (float)pLowerResSrc->width;
      w = m_uiMaxResolution;
      h = (ezUInt32)(pLowerResSrc->height * fAdjust);
    }
    else
    {
      const float fAdjust = (float)m_uiMaxResolution / (float)pLowerResSrc->height;
      w = (ezUInt32)(pLowerResSrc->width * fAdjust);
      h = m_uiMaxResolution;
    }

    // pick the closest multiple of 4 for the image size
    // this is important for some compression methods
    w = ezMath::RoundUp(w, 4U);
    h = ezMath::RoundUp(h, 4U);

    w = ezMath::Max(w, 4U);
    h = ezMath::Max(h, 4U);

    // TODO: snap resolution to power-of-two ?

    const HRESULT res = Resize(*pLowerResSrc, w, h, TEX_FILTER_TRIANGLE | TEX_FILTER_SEPARATE_ALPHA, temp);
    if (FAILED(res))
    {
      ezLog::Error("Failed to scale down image to maximum resolution ({0}x{1} -> {2}x{3})", pLowerResSrc->width, pLowerResSrc->height, w,
                   h);
      return EZ_FAILURE;
    }

    pLowerResSrc = temp.GetImage(0, 0, 0);
  }

  // copy the lower res image into the current image
  m_pCurrentImage = make_shared<ScratchImage>();
  m_pCurrentImage->InitializeFromImage(*pLowerResSrc);

  return EZ_SUCCESS;
}

ezImage* ezTexConv::CreateCombined2DImage(const ChannelMapping* dataSources)
{
  /// \todo Handle different input sizes

  ezImage* pImg = EZ_DEFAULT_NEW(ezImage);
  m_CleanupImages.PushBack(pImg);

  /// \todo Return loaded image pointer, if no combination is necessary

  const ezUInt32 uiWidth = m_InputImages[0].GetWidth();
  const ezUInt32 uiHeight = m_InputImages[0].GetHeight();

  ezImageHeader imgHeader;
  imgHeader.SetWidth(uiWidth);
  imgHeader.SetHeight(uiHeight);
  imgHeader.SetDepth(1);
  imgHeader.SetNumArrayIndices(1);
  imgHeader.SetNumFaces(1);
  imgHeader.SetNumMipLevels(1);
  imgHeader.SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  pImg->ResetAndAlloc(imgHeader);

  static_assert(sizeof(ezColor) == sizeof(float) * 4, "The loop below assumes that ezColor is 4 floats in size");

  // later block compression may pre-multiply rgb by alpha, if we have never set alpha to anything (3 channel case), that will result in
  // black
  ezColor defaultResult(0.0f, 0.0f, 0.0f, 0.0f);
  if (m_uiOutputChannels < 4)
    defaultResult.a = 1.0f;

  // not the most efficient loops...
  for (ezUInt32 h = 0; h < uiHeight; ++h)
  {
    for (ezUInt32 w = 0; w < uiWidth; ++w)
    {
      ezColor resultRGBA = defaultResult;

      for (ezUInt32 i = 0; i < m_uiOutputChannels; ++i)
      {
        const auto& ds = dataSources[i];

        float channelValue = 0.0f;

        if (ds.m_iInput == -1)
        {
          // handles 'black' and 'white' values

          if (ds.m_uiChannelMask == Channel::All)
            channelValue = 1.0f;
        }
        else
        {
          const ezImage& src = m_InputImages[ds.m_iInput];
          const ezColor rgba = *src.GetPixelPointer<ezColor>(0, 0, 0, w, h, 0);

          channelValue = GetChannelValue(ds, rgba);
        }

        // we build all images in linear space and set the SRGB format at the end (or not)
        resultRGBA.GetData()[i] = channelValue;
      }

      *pImg->GetPixelPointer<ezColor>(0, 0, 0, w, h, 0) = resultRGBA;
    }
  }

  return pImg;
}
