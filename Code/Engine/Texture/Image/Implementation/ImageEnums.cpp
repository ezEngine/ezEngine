#include <TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezImageAddressMode, 1)
  EZ_ENUM_CONSTANT(ezImageAddressMode::Repeat),
  EZ_ENUM_CONSTANT(ezImageAddressMode::Clamp),
  EZ_ENUM_CONSTANT(ezImageAddressMode::ClampBorder),
  EZ_ENUM_CONSTANT(ezImageAddressMode::Mirror),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTextureFilterSetting, 1)
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedNearest),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedBilinear),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedTrilinear),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedAnisotropic2x),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedAnisotropic4x),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedAnisotropic8x),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::FixedAnisotropic16x),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::LowestQuality),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::LowQuality),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::DefaultQuality),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::HighQuality),
  EZ_ENUM_CONSTANT(ezTextureFilterSetting::HighestQuality),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on
