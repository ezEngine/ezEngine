#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Tracks/Curve1D.h>

class ezCurve1D;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GUIFOUNDATION_DLL, ezCurveTangentMode);

template<typename T>
void FindNearestControlPoints(ezArrayPtr<T> cps, ezInt64 iTick, T*& llhs, T*& lhs, T*& rhs, T*& rrhs)
{
  llhs = nullptr;
  lhs = nullptr;
  rhs = nullptr;
  rrhs = nullptr;
  ezInt64 lhsTick = std::numeric_limits<ezInt64>::min();
  ezInt64 llhsTick = std::numeric_limits<ezInt64>::min();
  ezInt64 rhsTick = std::numeric_limits<ezInt64>::max();
  ezInt64 rrhsTick = std::numeric_limits<ezInt64>::max();

  for (decltype(auto) cp : cps)
  {
    if (cp.m_iTick <= iTick )
    {
      if (cp.m_iTick > lhsTick)
      {
        llhs = lhs;
        llhsTick = lhsTick;

        lhs = &cp;
        lhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick > llhsTick)
      {
        llhs = &cp;
        llhsTick = cp.m_iTick;
      }
    }

    if (cp.m_iTick > iTick)
    {
      if (cp.m_iTick < rhsTick)
      {
        rrhs = rhs;
        rrhsTick = rhsTick;

        rhs = &cp;
        rhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick < rrhsTick)
      {
        rrhs = &cp;
        rrhsTick = cp.m_iTick;
      }
    }
  }
}

class EZ_GUIFOUNDATION_DLL ezCurveControlPointData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurveControlPointData, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);

  ezInt64 m_iTick; // 4800 ticks per second
  double m_fValue;
  ezVec2 m_LeftTangent;
  ezVec2 m_RightTangent;
  bool m_bTangentsLinked = true;
  ezEnum<ezCurveTangentMode> m_LeftTangentMode;
  ezEnum<ezCurveTangentMode> m_RightTangentMode;
};

class EZ_GUIFOUNDATION_DLL ezSingleCurveData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSingleCurveData, ezReflectedClass);
public:
  ezColorGammaUB m_CurveColor;
  ezDynamicArray<ezCurveControlPointData> m_ControlPoints;

  void ConvertToRuntimeData(ezCurve1D& out_Result) const;
  double Evaluate(ezInt64 uiTick) const;
};

class EZ_GUIFOUNDATION_DLL ezCurveGroupData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurveGroupData, ezReflectedClass);
public:
  ezCurveGroupData() = default;
  ezCurveGroupData(const ezCurveGroupData& rhs) = delete;
  ~ezCurveGroupData();
  ezCurveGroupData& operator=(const ezCurveGroupData& rhs) = delete;

  /// \brief Makes a deep copy of rhs.
  void CloneFrom(const ezCurveGroupData& rhs);

  /// \brief Clears the curve and deallocates the curve data, if it is owned (e.g. if it was created through CloneFrom())
  void Clear();

  /// Can be set to false for cases where the instance is only supposed to act like a container for passing curve pointers around
  bool m_bOwnsData = true;
  ezDynamicArray<ezSingleCurveData*> m_Curves;
  ezUInt16 m_uiFramesPerSecond = 60;

  ezInt64 TickFromTime(double time);

  void ConvertToRuntimeData(ezUInt32 uiCurveIdx, ezCurve1D& out_Result) const;
};

struct EZ_GUIFOUNDATION_DLL ezSelectedCurveCP
{
  EZ_DECLARE_POD_TYPE();

  ezUInt16 m_uiCurve;
  ezUInt16 m_uiPoint;
};
