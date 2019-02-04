#pragma once

#include <Foundation/Basics.h>
#include <Texture/TexConv/TexConvEnums.h>
#include <Foundation/Types/Enum.h>

class ezStreamWriter;
class ezStreamReader;

struct EZ_TEXTURE_DLL ezTexFormat
{
  bool m_bSRGB = false;
  ezEnum<ezTexConvWrapMode> m_WrapModeU;
  ezEnum<ezTexConvWrapMode> m_WrapModeV;
  ezEnum<ezTexConvWrapMode> m_WrapModeW;

  // version 2
  ezEnum<ezTexConvFilterMode> m_TextureFilter;

  // version 3
  ezInt16 m_iRenderTargetResolutionX = 0;
  ezInt16 m_iRenderTargetResolutionY = 0;

  // version 4
  float m_fResolutionScale = 1.0f;

  // version 5
  int m_GalRenderTargetFormat = 0;

  void WriteTextureHeader(ezStreamWriter& stream) const;
  void WriteRenderTargetHeader(ezStreamWriter& stream) const;
  void ReadHeader(ezStreamReader& stream);
};

