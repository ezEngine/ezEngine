#pragma once

#include <Texture/TextureDLL.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_TEXTURE_DLL ezImageAddressMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Repeat,
    Clamp,
    ClampBorder,
    Mirror,

    ENUM_COUNT,

    Default = Repeat
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXTURE_DLL, ezImageAddressMode);

//////////////////////////////////////////////////////////////////////////
// ezTextureFilterSetting
//////////////////////////////////////////////////////////////////////////

struct EZ_TEXTURE_DLL ezTextureFilterSetting
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FixedNearest,
    FixedBilinear,
    FixedTrilinear,
    FixedAnisotropic2x,
    FixedAnisotropic4x,
    FixedAnisotropic8x,
    FixedAnisotropic16x,

    LowestQuality,
    LowQuality,
    DefaultQuality,
    HighQuality,
    HighestQuality,

    Default = DefaultQuality
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_TEXTURE_DLL, ezTextureFilterSetting);
