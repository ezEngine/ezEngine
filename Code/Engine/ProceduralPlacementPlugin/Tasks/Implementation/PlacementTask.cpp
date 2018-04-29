#include <PCH.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <Foundation/Math/Random.h>
#include <Foundation/SimdMath/SimdConversion.h>

using namespace ezPPInternal;

EZ_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
//EZ_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64); // TODO: Fails on Linux and Mac

PlacementTask::PlacementTask()
{
}

PlacementTask::~PlacementTask()
{
}

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
  if (m_pLayer->m_ByteCode != nullptr)
  {
    ezUInt32 uiNumInstances = m_InputPoints.GetCount();
    m_TempData.SetCountUninitialized(uiNumInstances * 5);

    ezExpression::Stream inputs[ExpressionInputs::Count];
    {
      for (ezUInt32 i = 0; i < 3; ++i) // Position
      {
        ezUInt8* ptr = reinterpret_cast<ezUInt8*>(m_InputPoints.GetData()) + (i * sizeof(float));
        inputs[ExpressionInputs::PositionX + i].m_Data = ezMakeArrayPtr(ptr, uiNumInstances * sizeof(PlacementPoint));
        inputs[ExpressionInputs::PositionX + i].m_uiByteStride = sizeof(PlacementPoint);
      }

      for (ezUInt32 i = 0; i < 3; ++i) // Normal
      {
        ezUInt8* ptr = reinterpret_cast<ezUInt8*>(m_InputPoints.GetData()) + ((i + 4) * sizeof(float));
        inputs[ExpressionInputs::NormalX + i].m_Data = ezMakeArrayPtr(ptr, uiNumInstances * sizeof(PlacementPoint));
        inputs[ExpressionInputs::NormalX + i].m_uiByteStride = sizeof(PlacementPoint);
      }

      // Point index
      ezArrayPtr<float> pointIndex = m_TempData.GetArrayPtr().GetSubArray(0, uiNumInstances);
      for (ezUInt32 i = 0; i < uiNumInstances; ++i)
      {
        pointIndex[i] = m_InputPoints[i].m_uiPointIndex;
      }
      inputs[ExpressionInputs::PointIndex].m_Data = pointIndex.ToByteArray();
      inputs[ExpressionInputs::PointIndex].m_uiByteStride = sizeof(float);
    }

    ezArrayPtr<float> density = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 1, uiNumInstances);
    ezArrayPtr<float> scale = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 2, uiNumInstances);
    ezArrayPtr<float> colorIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 3, uiNumInstances);
    ezArrayPtr<float> objectIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 4, uiNumInstances);

    ezExpression::Stream outputs[ExpressionOutputs::Count];
    {
      outputs[ExpressionOutputs::Density].m_Data = density.ToByteArray();
      outputs[ExpressionOutputs::Density].m_uiByteStride = sizeof(float);

      outputs[ExpressionOutputs::Scale].m_Data = scale.ToByteArray();
      outputs[ExpressionOutputs::Scale].m_uiByteStride = sizeof(float);

      outputs[ExpressionOutputs::ColorIndex].m_Data = colorIndex.ToByteArray();
      outputs[ExpressionOutputs::ColorIndex].m_uiByteStride = sizeof(float);

      outputs[ExpressionOutputs::ObjectIndex].m_Data = objectIndex.ToByteArray();
      outputs[ExpressionOutputs::ObjectIndex].m_uiByteStride = sizeof(float);
    }

    // Execute expression bytecode
    m_VM.Execute(*(m_pLayer->m_ByteCode), inputs, outputs, uiNumInstances);

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

  //TODO: faster random, simd optimize
  ezRandom rng;
  ezVec3 vUp = ezVec3(0, 0, 1);
  ezUInt8 uiMaxObjectIndex = (ezUInt8)(m_pLayer->m_ObjectsToPlace.GetCount() - 1);

  for (ezUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
  {
    ezUInt32 uiInputPointIndex = m_ValidPoints[i];
    auto& placementPoint = m_InputPoints[uiInputPointIndex];
    auto& placementTransform = m_OutputTransforms[i];

    rng.Initialize(placementPoint.m_uiPointIndex + 1);

    float offsetX = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.x, m_pLayer->m_vMaxOffset.x);
    float offsetY = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.y, m_pLayer->m_vMaxOffset.y);
    float offsetZ = (float)rng.DoubleMinMax(m_pLayer->m_vMinOffset.z, m_pLayer->m_vMaxOffset.z);
    ezSimdVec4f offset(offsetX, offsetY, offsetZ);

    ezAngle angle = ezAngle::Degree((float)rng.DoubleInRange(0.0f, 360.0f));
    ezQuat qYawRot; qYawRot.SetFromAxisAndAngle(vUp, angle);
    ezQuat qToNormalRot; qToNormalRot.SetShortestRotation(vUp, ezMath::Lerp(vUp, placementPoint.m_vNormal, m_pLayer->m_fAlignToNormal));

    ezVec3 scale = ezMath::Lerp(m_pLayer->m_vMinScale, m_pLayer->m_vMaxScale, ezMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));

    placementTransform.m_Transform.SetIdentity();
    placementTransform.m_Transform.m_Position = ezSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;
    placementTransform.m_Transform.m_Rotation = ezSimdConversion::ToQuat(qToNormalRot * qYawRot);
    placementTransform.m_Transform.m_Scale = ezSimdConversion::ToVec3(scale);

    float colorIndex = placementPoint.m_uiColorIndex / 255.0f;
    placementTransform.m_Color = ezMath::Lerp(ezColor::White, ezColor::Red, colorIndex);

    placementTransform.m_uiObjectIndex = ezMath::Min(placementPoint.m_uiObjectIndex, uiMaxObjectIndex);
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
  }
}
