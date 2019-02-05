#include <PCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezImageAddressMode, 1)
  EZ_ENUM_CONSTANT(ezImageAddressMode::Repeat),
  EZ_ENUM_CONSTANT(ezImageAddressMode::Clamp),
  EZ_ENUM_CONSTANT(ezImageAddressMode::ClampBorder),
  EZ_ENUM_CONSTANT(ezImageAddressMode::Mirror),
  EZ_ENUM_CONSTANT(ezImageAddressMode::MirrorOnce),
EZ_END_STATIC_REFLECTED_ENUM;



// clang-format on
