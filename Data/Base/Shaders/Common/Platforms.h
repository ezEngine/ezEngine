#pragma once

#include "StandardMacros.h"

#define PLATFORM_SHADER EZ_OFF
#define PLATFORM_DX11 EZ_OFF
#define PLATFORM_VULKAN EZ_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

  #undef PLATFORM_SHADER
  #define PLATFORM_SHADER EZ_ON

  #undef PLATFORM_DX11
  #define PLATFORM_DX11 EZ_ON

#endif

#if defined(VULKAN)

  #undef PLATFORM_SHADER
  #define PLATFORM_SHADER EZ_ON

  #undef PLATFORM_VULKAN
  #define PLATFORM_VULKAN EZ_ON

#endif
