#pragma once

#include <Foundation/Basics/Types.h>
#include <Foundation/Basics/TypeTraits.h>

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
