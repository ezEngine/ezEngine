#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Transform.h>

/// \brief A wrapper class that converts a ezMat3 into the correct data layout for shaders.
class ezShaderMat3
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezShaderMat3() = default;

  EZ_ALWAYS_INLINE ezShaderMat3(const ezMat3& m) { *this = m; }

  EZ_FORCE_INLINE void operator=(const ezMat3& m)
  {
    ezMemoryUtils::Copy(&m_Data[0], &m.m_fElementsCM[0], 3);
    m_Data[3] = 0.0f;

    ezMemoryUtils::Copy(&m_Data[4], &m.m_fElementsCM[3], 3);
    m_Data[7] = 0.0f;

    ezMemoryUtils::Copy(&m_Data[8], &m.m_fElementsCM[6], 3);
    m_Data[11] = 0.0f;
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a ezMat4 into the correct data layout for shaders.
class ezShaderMat4
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezShaderMat4() = default;

  EZ_ALWAYS_INLINE ezShaderMat4(const ezMat4& m) { *this = m; }

  EZ_FORCE_INLINE void operator=(const ezMat4& m)
  {
    ezMemoryUtils::Copy(m_Data, m.m_fElementsCM, 16);
  }

private:
  float m_Data[16];
};

/// \brief A wrapper class that converts a ezTransform into the correct data layout for shaders.
class ezShaderTransform
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezShaderTransform() = default;

  inline void operator=(const ezTransform& t) { *this = t.GetAsMat4(); }

  inline void operator=(const ezMat4& t)
  {
    float data[16];
    t.GetAsArray(data, ezMatrixLayout::RowMajor);

    for (ezUInt32 i = 0; i < 12; ++i)
    {
      m_Data[i] = data[i];
    }
  }

  inline void operator=(const ezMat3& t)
  {
    float data[9];
    t.GetAsArray(data, ezMatrixLayout::RowMajor);

    m_Data[0] = data[0];
    m_Data[1] = data[1];
    m_Data[2] = data[2];
    m_Data[3] = 0;

    m_Data[4] = data[3];
    m_Data[5] = data[4];
    m_Data[6] = data[5];
    m_Data[7] = 0;

    m_Data[8] = data[6];
    m_Data[9] = data[7];
    m_Data[10] = data[8];
    m_Data[11] = 0;
  }

  inline ezMat4 GetAsMat4() const
  {
    ezMat4 res;
    res.SetRow(0, reinterpret_cast<const ezVec4&>(m_Data[0]));
    res.SetRow(1, reinterpret_cast<const ezVec4&>(m_Data[4]));
    res.SetRow(2, reinterpret_cast<const ezVec4&>(m_Data[8]));
    res.SetRow(3, ezVec4(0, 0, 0, 1));

    return res;
  }

  inline ezVec3 GetTranslationVector() const
  {
    return ezVec3(m_Data[3], m_Data[7], m_Data[11]);
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a bool into the correct data layout for shaders.
class ezShaderBool
{
public:
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezShaderBool() = default;

  EZ_ALWAYS_INLINE ezShaderBool(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

  EZ_ALWAYS_INLINE void operator=(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

private:
  ezUInt32 m_uiData;
};
