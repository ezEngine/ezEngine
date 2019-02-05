#include <PCH.h>

#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::LoadInputImages()
{
  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    ezLog::Error("No input images have been specified.");
    return EZ_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    ezLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return EZ_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
  {
    // make sure the two arrays have the same size
    m_Descriptor.m_InputFiles.SetCount(m_Descriptor.m_InputImages.GetCount());

    ezStringBuilder tmp;
    for (ezUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
    {
      tmp.Format("InputImage{}", ezArgI(i, 2, true));
      m_Descriptor.m_InputFiles[i] = tmp;
    }
  }
  else
  {
    m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

    for (const auto& file : m_Descriptor.m_InputFiles)
    {
      auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
      if (img.LoadFrom(file).Failed())
      {
        ezLog::Error("Could not load input file '{0}'.", file);
        return EZ_FAILURE;
      }
    }
  }

  for (ezUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == ezImageFormat::UNKNOWN)
    {
      ezLog::Error("Unknown image format for '{}'", m_Descriptor.m_InputFiles[i]);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ConvertAndScaleImage(
  const char* szImageName, ezImage& inout_Image, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  if (inout_Image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    ezLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", szImageName);
    return EZ_FAILURE;
  }

  if (ezImageUtils::Scale(
        inout_Image, inout_Image, uiResolutionX, uiResolutionY, nullptr, ezImageAddressMode::Clamp, ezImageAddressMode::Clamp)
        .Failed())
  {
    ezLog::Error("Could not resize '{}' to {}x{}", szImageName, uiResolutionX, uiResolutionY);
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ConvertAndScaleInputImages(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY)
{
  for (ezUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];
    const char* szName = m_Descriptor.m_InputFiles[idx];

    EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(szName, img, uiResolutionX, uiResolutionY));
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_InputFiles);
