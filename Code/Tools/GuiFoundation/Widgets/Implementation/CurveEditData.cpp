#include <PCH.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Curve1D.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCurveTangentMode, 1)
EZ_ENUM_CONSTANTS(ezCurveTangentMode::Bezier, ezCurveTangentMode::FixedLength, ezCurveTangentMode::Linear, ezCurveTangentMode::Auto)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurveControlPointData, 5, ezRTTIDefaultAllocator<ezCurveControlPointData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_MEMBER_PROPERTY("Value", m_fValue),
    EZ_MEMBER_PROPERTY("LeftTangent", m_LeftTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(-0.1f, 0))),
    EZ_MEMBER_PROPERTY("RightTangent", m_RightTangent)->AddAttributes(new ezDefaultValueAttribute(ezVec2(+0.1f, 0))),
    EZ_MEMBER_PROPERTY("Linked", m_bTangentsLinked)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("LeftTangentMode", ezCurveTangentMode, m_LeftTangentMode),
    EZ_ENUM_MEMBER_PROPERTY("RightTangentMode", ezCurveTangentMode, m_RightTangentMode),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSingleCurveData, 3, ezRTTIDefaultAllocator<ezSingleCurveData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_CurveColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor::GreenYellow))),
    EZ_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurveGroupData, 2, ezRTTIDefaultAllocator<ezCurveGroupData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Curves", m_Curves),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


void ezCurveControlPointData::SetTickFromTime(double time, ezInt64 fps)
{
  m_iTick = (ezInt64)ezMath::Round(time * 4800.0, (double)fps);
}

ezInt64 ezCurveGroupData::TickFromTime(double time)
{
  return (ezInt64)ezMath::Round(time * 4800.0, (double)m_uiFramesPerSecond);
}

void ezSingleCurveData::ConvertToRuntimeData(ezCurve1D& out_Result) const
{
  out_Result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    auto& ccp = out_Result.AddControlPoint(cp.GetTickAsTime());
    ccp.m_Position.y = cp.m_fValue;
    ccp.m_LeftTangent = cp.m_LeftTangent;
    ccp.m_RightTangent = cp.m_RightTangent;
    ccp.m_TangentModeLeft = cp.m_LeftTangentMode;
    ccp.m_TangentModeRight = cp.m_RightTangentMode;
  }
}

void ezCurveGroupData::ConvertToRuntimeData(ezUInt32 uiCurveIdx, ezCurve1D& out_Result) const
{
  m_Curves[uiCurveIdx].ConvertToRuntimeData(out_Result);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezCurve1DControlPoint_2_3 : public ezGraphPatch
{
public:
  ezCurve1DControlPoint_2_3()
    : ezGraphPatch("ezCurve1DControlPoint", 3) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Point");
    if (pPoint && pPoint->m_Value.IsA<ezVec2>())
    {
      ezVec2 pt = pPoint->m_Value.Get<ezVec2>();
      pNode->AddProperty("Time", (double)ezMath::Max(0.0f, pt.x));
      pNode->AddProperty("Value", (double)pt.y);
      pNode->AddProperty("LeftTangentMode", (ezUInt32)ezCurveTangentMode::Bezier);
      pNode->AddProperty("RightTangentMode", (ezUInt32)ezCurveTangentMode::Bezier);
    }
  }
};

ezCurve1DControlPoint_2_3 g_ezCurve1DControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class ezCurve1DControlPoint_3_4 : public ezGraphPatch
{
public:
  ezCurve1DControlPoint_3_4()
    : ezGraphPatch("ezCurve1DControlPoint", 4) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Time");
    if (pPoint && pPoint->m_Value.IsA<double>())
    {
      const double fTime = pPoint->m_Value.Get<double>();
      pNode->AddProperty("Tick", (ezInt64)ezMath::Round(fTime * 4800.0, 60.0));
    }
  }
};

ezCurve1DControlPoint_3_4 g_ezCurve1DControlPoint_3_4;

//////////////////////////////////////////////////////////////////////////

class ezCurve1DControlPoint_4_5 : public ezGraphPatch
{
public:
  ezCurve1DControlPoint_4_5()
    : ezGraphPatch("ezCurve1DControlPoint", 5) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.RenameClass("ezCurveControlPointData");
  }
};

ezCurve1DControlPoint_4_5 g_ezCurve1DControlPoint_4_5;

//////////////////////////////////////////////////////////////////////////

class ezCurve1DData_2_3 : public ezGraphPatch
{
public:
  ezCurve1DData_2_3()
    : ezGraphPatch("ezCurve1DData", 3) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.RenameClass("ezSingleCurveData");
  }
};

ezCurve1DData_2_3 g_ezCurve1DData_2_3;

//////////////////////////////////////////////////////////////////////////

class ezCurve1DAssetData_1_2 : public ezGraphPatch
{
public:
  ezCurve1DAssetData_1_2()
    : ezGraphPatch("ezCurve1DAssetData", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.RenameClass("ezCurveGroupData");
  }
};

ezCurve1DAssetData_1_2 g_ezCurve1DAssetData_1_2;
