#pragma once

#include "StandardMacros.h"

#define PLATFORM_DX11 EZ_OFF
#define PLATFORM_OPENGL EZ_OFF

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

  #undef PLATFORM_DX11
  #define PLATFORM_DX11 EZ_ON

#endif

#if defined(GL3) || defined(GL4)

  #version 430

  #undef PLATFORM_OPENGL
  #define PLATFORM_OPENGL EZ_ON

#endif

