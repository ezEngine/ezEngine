#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Containers/HybridArray.h>

class ezStreamWriter;
class ezStreamReader;

class EZ_FOUNDATION_DLL ezColorGradient
{
public:
  struct ColorCP
  {
    EZ_DECLARE_POD_TYPE();

    float m_PosX;
    ezUInt8 m_GammaRed;
    ezUInt8 m_GammaGreen;
    ezUInt8 m_GammaBlue;

    EZ_FORCE_INLINE bool operator<(const ColorCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  struct AlphaCP
  {
    EZ_DECLARE_POD_TYPE();

    float m_PosX;
    ezUInt8 m_Alpha;

    EZ_FORCE_INLINE bool operator<(const AlphaCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  struct IntensityCP
  {
    EZ_DECLARE_POD_TYPE();

    float m_PosX;
    float m_Intensity;

    EZ_FORCE_INLINE bool operator<(const IntensityCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

public:
  ezColorGradient();

  void Clear();
  bool IsEmpty() const;

  void AddColorControlPoint(float x, const ezColorGammaUB& rgb);
  void AddAlphaControlPoint(float x, ezUInt8 alpha);
  void AddIntensityControlPoint(float x, float intensity);

  bool GetExtents(float& minx, float& maxx) const;

  void GetNumControlPoints(ezUInt32& rgb, ezUInt32& alpha, ezUInt32& intensity) const;

  const ColorCP& GetColorControlPoint(ezUInt32 idx) const { return m_ColorCPs[idx]; }
  const AlphaCP& GetAlphaControlPoint(ezUInt32 idx) const { return m_AlphaCPs[idx]; }
  const IntensityCP& GetIntensityControlPoint(ezUInt32 idx) const { return m_IntensityCPs[idx]; }

  ColorCP& ModifyColorControlPoint(ezUInt32 idx) { return m_ColorCPs[idx]; }
  AlphaCP& ModifyAlphaControlPoint(ezUInt32 idx) { return m_AlphaCPs[idx]; }
  IntensityCP& ModifyIntensityControlPoint(ezUInt32 idx) { return m_IntensityCPs[idx]; }

  void SortControlPoints();

  void Evaluate(float x, ezColorGammaUB& rgba, float& intensity) const;

  void EvaluateColor(float x, ezColorGammaUB& rgb) const;
  void EvaluateAlpha(float x, ezUInt8& alpha) const;
  void EvaluateIntensity(float x, float& intensity) const;

  ezUInt64 GetHeapMemoryUsage() const;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

private:

  ezHybridArray<ColorCP, 8> m_ColorCPs;
  ezHybridArray<AlphaCP, 8> m_AlphaCPs;
  ezHybridArray<IntensityCP, 8> m_IntensityCPs;
};