#pragma once

#if defined(DX11_SM40_93) || defined(DX11_SM40) || defined(DX11_SM41) || defined(DX11_SM50)

  #define PLATFORM_DX11
  
#endif

#if defined(GL3) || defined(GL4)

  #version 430

  #define PLATFORM_OPENGL

#endif

