#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief Runtime texture format metadata stored in ezTex files.
///
/// This structure contains all the metadata needed by the renderer to properly sample
/// and use textures at runtime. It's saved as a header in .ezTex files and loaded
/// by the texture resource system.
struct EZ_TEXTURE_DLL ezTexFormat
{
  bool m_bSRGB = false;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;

  // version 2
  ezEnum<ezTextureFilterSetting> m_TextureFilter;

  // Version 3 additions - Render target specific
  ezInt16 m_iRenderTargetResolutionX = 0; ///< Fixed render target width (0 = dynamic)
  ezInt16 m_iRenderTargetResolutionY = 0; ///< Fixed render target height (0 = dynamic)

  // Version 4 additions
  float m_fResolutionScale = 1.0f; ///< Resolution scaling factor for dynamic render targets

  // Version 5 additions
  int m_GalRenderTargetFormat = 0; ///< Graphics abstraction layer format for render targets

  void WriteTextureHeader(ezStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(ezStreamWriter& inout_stream) const;
  void ReadHeader(ezStreamReader& inout_stream);
};
