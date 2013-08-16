#pragma once

#include <Foundation/Basics.h>

/// \brief A class containing 8bit red, green, blue and alpha components.
struct ezRgba
{
  EZ_DECLARE_POD_TYPE();

  ezRgba()
  {
  }

  ezRgba(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a) :
    r(r), g(g), b(b), a(a)
  {
  }

  ezUInt8 r;
  ezUInt8 g;
  ezUInt8 b;
  ezUInt8 a;
};

EZ_CHECK_AT_COMPILETIME(sizeof(ezRgba) == 4);

/// \brief A class containing 32bit floating point red, green, blue and alpha components.
struct ezRgbaF
{
  EZ_DECLARE_POD_TYPE();

  ezRgbaF()
  {
  }

  ezRgbaF(float r, float g, float b, float a) :
    r(r), g(g), b(b), a(a)
  {
  }

  float r;
  float g;
  float b;
  float a;
};

EZ_CHECK_AT_COMPILETIME(sizeof(ezRgbaF) == 16);

/// \brief A class containing 8bit blue, green, red and alpha components.
struct ezBgra
{
  EZ_DECLARE_POD_TYPE();

  ezBgra()
  {
  }

  ezBgra(ezUInt8 b, ezUInt8 g, ezUInt8 r, ezUInt8 a) :
    b(b), g(g), r(r), a(a)
  {
  }

  ezUInt8 b;
  ezUInt8 g;
  ezUInt8 r;
  ezUInt8 a;
};

EZ_CHECK_AT_COMPILETIME(sizeof(ezBgra) == 4);
