#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class ezStreamWriter;
class ezStreamReader;

struct EZ_TEXTURE_DLL ezTexFormat
{
  bool m_bSRGB = false;
  ezEnum<ezImageAddressMode> m_AddressModeU;
  ezEnum<ezImageAddressMode> m_AddressModeV;
  ezEnum<ezImageAddressMode> m_AddressModeW;

  // version 2
  ezEnum<ezTextureFilterSetting> m_TextureFilter;

  // version 3
  ezInt16 m_iRenderTargetResolutionX = 0;
  ezInt16 m_iRenderTargetResolutionY = 0;

  // version 4
  float m_fResolutionScale = 1.0f;

  // version 5
  int m_GalRenderTargetFormat = 0;

  void WriteTextureHeader(ezStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(ezStreamWriter& inout_stream) const;
  void ReadHeader(ezStreamReader& inout_stream);
};
