#pragma once

#include <RendererCore/Basics.h>

/// \brief A wrapper class that converts a ezMat3 into the correct data layout for shaders.
class ezShaderMat3
{
public:
  EZ_FORCE_INLINE ezShaderMat3()
  {
  }

  EZ_FORCE_INLINE ezShaderMat3(const ezMat3& m)
  {
    *this = m;
  }

  EZ_FORCE_INLINE void operator=(const ezMat3& m)
  {
    for (ezUInt32 c = 0; c < 3; ++c)
    {
      m_Data[c * 4 + 0] = m.Element(c, 0);
      m_Data[c * 4 + 1] = m.Element(c, 1);
      m_Data[c * 4 + 2] = m.Element(c, 2);
      m_Data[c * 4 + 3] = 0.0f;
    }
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a ezTransform into the correct data layout for shaders.
class ezShaderTransform
{
public:
  EZ_FORCE_INLINE ezShaderTransform()
  {
  }

  EZ_FORCE_INLINE ezShaderTransform(const ezTransform& t)
  {
    *this = t;
  }

  EZ_FORCE_INLINE void operator=(const ezTransform& t)
  {
    t.GetAsArray(m_Data, ezMatrixLayout::RowMajor);
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a bool into the correct data layout for shaders.
class ezShaderBool
{
public:
  EZ_FORCE_INLINE ezShaderBool()
  {
  }

  EZ_FORCE_INLINE ezShaderBool(bool b)
  {
    m_Data = b ? 0xFFFFFFFF : 0;
  }

  EZ_FORCE_INLINE void operator=(bool b)
  {
    m_Data = b ? 0xFFFFFFFF : 0;
  }

private:
  ezUInt32 m_Data;
};
