#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezCurve1D;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GUIFOUNDATION_DLL, ezCurveTangentMode);

template <typename T>
void FindNearestControlPoints(ezArrayPtr<T> cps, ezInt64 iTick, T*& ref_pLlhs, T*& lhs, T*& rhs, T*& ref_pRrhs)
{
  ref_pLlhs = nullptr;
  lhs = nullptr;
  rhs = nullptr;
  ref_pRrhs = nullptr;
  ezInt64 lhsTick = ezMath::MinValue<ezInt64>();
  ezInt64 llhsTick = ezMath::MinValue<ezInt64>();
  ezInt64 rhsTick = ezMath::MaxValue<ezInt64>();
  ezInt64 rrhsTick = ezMath::MaxValue<ezInt64>();

  for (decltype(auto) cp : cps)
  {
    if (cp.m_iTick <= iTick)
    {
      if (cp.m_iTick > lhsTick)
      {
        ref_pLlhs = lhs;
        llhsTick = lhsTick;

        lhs = &cp;
        lhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick > llhsTick)
      {
        ref_pLlhs = &cp;
        llhsTick = cp.m_iTick;
      }
    }

    if (cp.m_iTick > iTick)
    {
      if (cp.m_iTick < rhsTick)
      {
        ref_pRrhs = rhs;
        rrhsTick = rhsTick;

        rhs = &cp;
        rhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick < rrhsTick)
      {
        ref_pRrhs = &cp;
        rrhsTick = cp.m_iTick;
      }
    }
  }
}

class EZ_GUIFOUNDATION_DLL ezCurveControlPointData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurveControlPointData, ezReflectedClass);

public:
  ezTime GetTickAsTime() const { return ezTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(ezTime time, ezInt64 iFps);

  ezInt64 m_iTick; // 4800 ticks per second
  double m_fValue;
  ezVec2 m_LeftTangent = ezVec2(-0.1f, 0.0f);
  ezVec2 m_RightTangent = ezVec2(+0.1f, 0.0f);
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

  void ConvertToRuntimeData(ezCurve1D& out_result) const;
  double Evaluate(ezInt64 iTick) const;
};

class EZ_GUIFOUNDATION_DLL ezCurveExtentsAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCurveExtentsAttribute, ezPropertyAttribute);

public:
  ezCurveExtentsAttribute() = default;
  ezCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed);

  double m_fLowerExtent = 0.0;
  double m_fUpperExtent = 1.0;
  bool m_bLowerExtentFixed = false;
  bool m_bUpperExtentFixed = false;
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

  ezInt64 TickFromTime(ezTime time) const;

  void ConvertToRuntimeData(ezUInt32 uiCurveIdx, ezCurve1D& out_result) const;
};

struct EZ_GUIFOUNDATION_DLL ezSelectedCurveCP
{
  EZ_DECLARE_POD_TYPE();

  ezUInt16 m_uiCurve;
  ezUInt16 m_uiPoint;
};
