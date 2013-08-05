#pragma once

#include <Foundation/Basics/Types.h>
#include <Foundation/Basics/TypeTraits.h>

struct ezRgba
{
  EZ_DECLARE_POD_TYPE();

  ezRgba()
  {

  }

  ezRgba(ezUInt8 red, ezUInt8 green, ezUInt8 blue, ezUInt8 alpha) :
    m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
  {

  }

  ezUInt8 m_red;
  ezUInt8 m_green;
  ezUInt8 m_blue;
  ezUInt8 m_alpha;
};
