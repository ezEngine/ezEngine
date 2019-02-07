#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Texture/Image/Image.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct EZ_RENDERERCORE_DLL ezTextureUtils
{
  static ezGALResourceFormat::Enum ImageFormatToGalFormat(ezImageFormat::Enum format, bool bSRGB);

  static void ConfigureSampler(ezTextureFilterSetting::Enum filter, ezGALSamplerStateCreationDescription& out_Sampler);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};

