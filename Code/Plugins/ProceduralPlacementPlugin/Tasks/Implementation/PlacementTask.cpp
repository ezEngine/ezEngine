#include <PCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>

using namespace ezPPInternal;

EZ_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
// EZ_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64); // TODO: Fails on Linux and Mac

namespace
{
  template <typename T>
  ezExpression::Stream MakeStream(ezArrayPtr<T> data, ezUInt32 uiOffset, const ezHashedString& sName)
  {
    auto byteData = data.ToByteArray().GetSubArray(uiOffset);

    return ezExpression::Stream(sName, ezExpression::Stream::Type::Float, byteData, sizeof(T));
  }
}

PlacementTask::PlacementTask() {}

PlacementTask::~PlacementTask() {}

void ezPPInternal::PlacementTask::Clear()
{
  m_pLayer = nullptr;
  m_InputPoints.Clear();
  m_OutputTransforms.Clear();
  m_TempData.Clear();
  m_ValidPoints.Clear();
}

void PlacementTask::Execute()
{
  // Execute bytecode
  if (m_pLayer->m_pByteCode != nullptr)
  {
    ezUInt32 uiNumInstances = m_InputPoints.GetCount();
    m_TempData.SetCountUninitialized(uiNumInstances * 5);

    ezHybridArray<ezExpression::Stream, 8> inputs;
    {
      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.x), ezPPInternal::ExpressionInputs::s_sPositionX));
      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.y), ezPPInternal::ExpressionInputs::s_sPositionY));
      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.z), ezPPInternal::ExpressionInputs::s_sPositionZ));

      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.x), ezPPInternal::ExpressionInputs::s_sNormalX));
      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.y), ezPPInternal::ExpressionInputs::s_sNormalY));
      inputs.PushBack(
          MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.z), ezPPInternal::ExpressionInputs::s_sNormalZ));

      // Point index
      ezArrayPtr<float> pointIndex = m_TempData.GetArrayPtr().GetSubArray(0, uiNumInstances);
      for (ezUInt32 i = 0; i < uiNumInstances; ++i)
      {
        pointIndex[i] = m_InputPoints[i].m_uiPointIndex;
      }
      inputs.PushBack(MakeStream(pointIndex, 0, ezPPInternal::ExpressionInputs::s_sPointIndex));
    }

    ezArrayPtr<float> density = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 1, uiNumInstances);
    ezArrayPtr<float> scale = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 2, uiNumInstances);
    ezArrayPtr<float> colorIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 3, uiNumInstances);
    ezArrayPtr<float> objectIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 4, uiNumInstances);

    ezHybridArray<ezExpression::Stream, 8> outputs;
    {
      outputs.PushBack(MakeStream(density, 0, ezPPInternal::ExpressionOutputs::s_sDensity));
      outputs.PushBack(MakeStream(scale, 0, ezPPInternal::ExpressionOutputs::s_sScale));
      outputs.PushBack(MakeStream(colorIndex, 0, ezPPInternal::ExpressionOutputs::s_sColorIndex));
      outputs.PushBack(MakeStream(objectIndex, 0, ezPPInternal::ExpressionOutputs::s_sObjectIndex));
    }

    // Execute expression bytecode
    m_VM.Execute(*(m_pLayer->m_pByteCode), inputs, outputs, uiNumInstances);

    // Test density against point threshold and fill remaining input point data from expression
    int iMaxObjectIndex = m_pLayer->m_ObjectsToPlace.GetCount() - 1;
    const Pattern* pPattern = m_pLayer->m_pPattern;
    for (ezUInt32 i = 0; i < uiNumInstances; ++i)
    {
      auto& inputPoint = m_InputPoints[i];
      ezUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
      float fThreshold = pPattern->m_Points[uiPointIndex].m_fThreshold;

      if (density[i] >= fThreshold)
      {
        inputPoint.m_fScale = scale[i];
        inputPoint.m_uiColorIndex = (ezUInt8)ezMath::Clamp(colorIndex[i], 0.0f, 255.0f);
        inputPoint.m_uiObjectIndex = (ezUInt8)ezMath::Clamp(static_cast<int>(objectIndex[i]), 0, iMaxObjectIndex);

        m_ValidPoints.PushBack(i);
      }
    }
  }

  if (m_ValidPoints.IsEmpty())
  {
    return;
  }

  // Construct final transforms
  m_OutputTransforms.SetCountUninitialized(m_ValidPoints.GetCount());

  ezSimdVec4i seed = ezSimdVec4i(m_iTileSeed) + ezSimdVec4i(0, 3, 7, 11);

  float fMinAngle = 0.0f;
  float fMaxAngle = ezMath::BasicType<float>::Pi() * 2.0f;

  ezSimdVec4f vMinValue = ezSimdVec4f(fMinAngle, m_pLayer->m_vMinOffset.z, 0.0f);
  ezSimdVec4f vMaxValue = ezSimdVec4f(fMaxAngle, m_pLayer->m_vMaxOffset.z, 0.0f);
  ezSimdVec4f vUp = ezSimdVec4f(0, 0, 1);
  ezSimdVec4f vAlignToNormal = ezSimdVec4f(m_pLayer->m_fAlignToNormal);
  ezSimdVec4f vMinScale = ezSimdConversion::ToVec3(m_pLayer->m_vMinScale);
  ezSimdVec4f vMaxScale = ezSimdConversion::ToVec3(m_pLayer->m_vMaxScale);
  ezUInt8 uiMaxObjectIndex = (ezUInt8)(m_pLayer->m_ObjectsToPlace.GetCount() - 1);

  const ezColorGradient* pColorGradient = nullptr;
  if (m_pLayer->m_hColorGradient.IsValid())
  {
    ezResourceLock<ezColorGradientResource> pColorGradientResource(m_pLayer->m_hColorGradient, ezResourceAcquireMode::NoFallback);
    pColorGradient = &(pColorGradientResource->GetDescriptor().m_Gradient);
  }

  for (ezUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
  {
    ezUInt32 uiInputPointIndex = m_ValidPoints[i];
    auto& placementPoint = m_InputPoints[uiInputPointIndex];
    auto& placementTransform = m_OutputTransforms[i];

    ezSimdVec4f random = ezSimdRandom::FloatMinMax(seed + ezSimdVec4i(placementPoint.m_uiPointIndex), vMinValue, vMaxValue);

    placementTransform.m_Transform.SetIdentity();

    ezSimdVec4f offset = ezSimdVec4f::ZeroVector();
    offset.SetZ(random.y());
    placementTransform.m_Transform.m_Position = ezSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;

    ezSimdQuat qYawRot;
    qYawRot.SetFromAxisAndAngle(vUp, random.x());
    ezSimdVec4f vNormal = ezSimdConversion::ToVec3(placementPoint.m_vNormal);
    ezSimdQuat qToNormalRot;
    qToNormalRot.SetShortestRotation(vUp, ezSimdVec4f::Lerp(vUp, vNormal, vAlignToNormal));
    placementTransform.m_Transform.m_Rotation = qToNormalRot * qYawRot;

    ezSimdVec4f scale = ezSimdVec4f(ezMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));
    placementTransform.m_Transform.m_Scale = ezSimdVec4f::Lerp(vMinScale, vMaxScale, scale);

    ezColorGammaUB objectColor = ezColor::White;
    if (pColorGradient != nullptr)
    {
      float colorIndex = placementPoint.m_uiColorIndex / 255.0f;
      ezUInt8 alpha;
      pColorGradient->EvaluateColor(colorIndex, objectColor);
      pColorGradient->EvaluateAlpha(colorIndex, alpha);
      objectColor.a = alpha;
    }
    placementTransform.m_Color = objectColor;

    placementTransform.m_uiObjectIndex = ezMath::Min(placementPoint.m_uiObjectIndex, uiMaxObjectIndex);
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
  }
}
