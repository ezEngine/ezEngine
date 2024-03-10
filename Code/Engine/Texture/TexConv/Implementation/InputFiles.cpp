#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::LoadInputImages()
{
  EZ_PROFILE_SCOPE("Load Images");

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
      tmp.SetFormat("InputImage{}", ezArgI(i, 2, true));
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
        ezLog::Error("Could not load input file '{0}'.", ezArgSensitive(file, "File"));
        return EZ_FAILURE;
      }
    }
  }

  for (ezUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == ezImageFormat::UNKNOWN)
    {
      ezLog::Error("Unknown image format for '{}'", ezArgSensitive(m_Descriptor.m_InputFiles[i], "File"));
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ConvertAndScaleImage(ezStringView sImageName, ezImage& inout_Image, ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezEnum<ezTexConvUsage> usage)
{
  const bool bSingleChannel = ezImageFormat::GetNumChannels(inout_Image.GetImageFormat()) == 1;

  if (inout_Image.Convert(ezImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    ezLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", sImageName);
    return EZ_FAILURE;
  }

  // some scale operations fail when they are done in place, so use a scratch image as destination for now
  ezImage scratch;
  if (ezImageUtils::Scale(inout_Image, scratch, uiResolutionX, uiResolutionY, nullptr, ezImageAddressMode::Clamp, ezImageAddressMode::Clamp).Failed())
  {
    ezLog::Error("Could not resize '{}' to {}x{}", sImageName, uiResolutionX, uiResolutionY);
    return EZ_FAILURE;
  }

  inout_Image.ResetAndMove(std::move(scratch));

  if (usage == ezTexConvUsage::Color && bSingleChannel)
  {
    // replicate single channel ("red" textures) into the other channels
    EZ_SUCCEED_OR_RETURN(ezImageUtils::CopyChannel(inout_Image, 1, inout_Image, 0));
    EZ_SUCCEED_OR_RETURN(ezImageUtils::CopyChannel(inout_Image, 2, inout_Image, 0));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ConvertAndScaleInputImages(ezUInt32 uiResolutionX, ezUInt32 uiResolutionY, ezEnum<ezTexConvUsage> usage)
{
  EZ_PROFILE_SCOPE("ConvertAndScaleInputImages");

  for (ezUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];
    ezStringView sName = m_Descriptor.m_InputFiles[idx];

    EZ_SUCCEED_OR_RETURN(ConvertAndScaleImage(sName, img, uiResolutionX, uiResolutionY, usage));
  }

  return EZ_SUCCESS;
}
