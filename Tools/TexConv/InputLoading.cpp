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

namespace
{
  bool isPowerOfTwo(ezUInt32 i)
  {
    // http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
    return i && !(i & (i - 1));
  }
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
    if (m_InputImages[i].GetWidth() != m_InputImages[0].GetWidth() ||
        m_InputImages[i].GetHeight() != m_InputImages[0].GetHeight())
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

ezResult ezTexConv::ConvertInputsToRGBA()
{
  for (ezUInt32 i = 0; i < m_InputImages.GetCount(); ++i)
  {
    if (ezImageConversion::Convert(m_InputImages[i], m_InputImages[i], ezImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      SetReturnCode(TexConvReturnCodes::FAILED_CONVERT_INPUT_TO_RGBA);
      ezLog::Error("Failed to convert input {0} from format {1} to R32G32B32A32_FLOAT. Format is not supported.", i, ezImageFormat::GetName(m_InputImages[i].GetImageFormat()));
      return EZ_FAILURE;
    }
  }

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

  pImg->SetWidth(uiWidth);
  pImg->SetHeight(uiHeight);
  pImg->SetDepth(1);
  pImg->SetNumArrayIndices(1);
  pImg->SetNumFaces(1);
  pImg->SetNumMipLevels(1);
  pImg->SetImageFormat(ezImageFormat::R32G32B32A32_FLOAT);
  pImg->AllocateImageData();

  static_assert(sizeof(ezColor) == sizeof(float) * 4, "The loop below assumes that ezColor is 4 floats in size");

  // later block compression may pre-multiply rgb by alpha, if we have never set alpha to anything (3 channel case), that will result in black
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
