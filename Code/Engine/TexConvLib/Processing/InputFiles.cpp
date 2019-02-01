#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

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

ezResult ezTexConvProcessor::ConvertInputImagesToFloat32()
{
  for (ezUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];

    if (img.Convert(ezImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      ezLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", m_Descriptor.m_InputFiles[idx]);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ResizeInputImagesToSameDimensions()
{
  for (ezUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];

    if (ezImageUtils::Scale(img, img, m_uiTargetResolutionX, m_uiTargetResolutionY, nullptr, ezImageAddressMode::CLAMP, ezImageAddressMode::CLAMP).Failed())
    {
      ezLog::Error("Could not resize '{}' to {}x{}", m_Descriptor.m_InputFiles[idx], m_uiTargetResolutionX, m_uiTargetResolutionY);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == ezTexConvUsage::Color || m_Descriptor.m_Usage == ezTexConvUsage::Compressed_4_Channel_sRGB ||
      m_Descriptor.m_Usage == ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel_SRGB)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (ezUInt32 i = 0; i < 3; ++i)
      {
        const ezInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(ezImageFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return EZ_SUCCESS;
}
