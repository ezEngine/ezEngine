#include <ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/Physics/SurfaceResource.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

using namespace ezProcGenInternal;

EZ_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
// EZ_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64); // TODO: Fails on Linux and Mac

PlacementTask::PlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, ezTaskNesting::Maybe);

  m_VM.RegisterDefaultFunctions();
  m_VM.RegisterFunction("ApplyVolumes", &ezProcGenExpressionFunctions::ApplyVolumes, &ezProcGenExpressionFunctions::ApplyVolumesValidate);
}

PlacementTask::~PlacementTask() = default;

void PlacementTask::Clear()
{
  m_InputPoints.Clear();
  m_OutputTransforms.Clear();
  m_TempData.Clear();
  m_ValidPoints.Clear();
}

void PlacementTask::Execute()
{
  FindPlacementPoints();

  if (!m_InputPoints.IsEmpty())
  {
    ExecuteVM();
  }
}

void PlacementTask::FindPlacementPoints()
{
  EZ_PROFILE_SCOPE("FindPlacementPoints");

  EZ_ASSERT_DEV(m_pData->m_pPhysicsModule != nullptr, "Physics module must be valid");
  auto pOutput = m_pData->m_pOutput;

  ezSimdVec4u seed = ezSimdVec4u(m_pData->m_iTileSeed) + ezSimdVec4u(0, 3, 7, 11);

  float fZRange = m_pData->m_TileBoundingBox.GetExtents().z;
  ezSimdFloat fZStart = m_pData->m_TileBoundingBox.m_vMax.z;
  ezSimdVec4f vXY = ezSimdConversion::ToVec3(m_pData->m_TileBoundingBox.m_vMin);
  ezSimdVec4f vMinOffset = ezSimdConversion::ToVec3(pOutput->m_vMinOffset);
  ezSimdVec4f vMaxOffset = ezSimdConversion::ToVec3(pOutput->m_vMaxOffset);

  ezVec3 rayDir = ezVec3(0, 0, -1);
  ezUInt32 uiCollisionLayer = pOutput->m_uiCollisionLayer;

  auto& patternPoints = pOutput->m_pPattern->m_Points;

  for (ezUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    auto& patternPoint = patternPoints[i];
    ezSimdVec4f patternCoords = ezSimdConversion::ToVec3(patternPoint.m_Coordinates.GetAsVec3(0.0f));

    ezSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
    rayStart += ezSimdRandom::FloatMinMax(seed + ezSimdVec4u(i), vMinOffset, vMaxOffset);
    rayStart.SetZ(fZStart);

    ezPhysicsCastResult hitResult;
    if (!m_pData->m_pPhysicsModule->Raycast(hitResult, ezSimdConversion::ToVec3(rayStart), rayDir, fZRange, ezPhysicsQueryParameters(uiCollisionLayer, ezPhysicsShapeType::Static)))
      continue;

    if (pOutput->m_hSurface.IsValid())
    {
      if (!hitResult.m_hSurface.IsValid())
        continue;

      ezResourceLock<ezSurfaceResource> hitSurface(hitResult.m_hSurface, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (hitSurface.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
        continue;

      if (!hitSurface->IsBasedOn(pOutput->m_hSurface))
        continue;
    }

    bool bInBoundingBox = false;
    ezSimdVec4f hitPosition = ezSimdConversion::ToVec3(hitResult.m_vPosition);
    ezSimdVec4f allOne = ezSimdVec4f(1.0f);
    for (auto& globalToLocalBox : m_pData->m_GlobalToLocalBoxTransforms)
    {
      ezSimdVec4f localHitPosition = globalToLocalBox.TransformPosition(hitPosition).Abs();
      if ((localHitPosition <= allOne).AllSet<3>())
      {
        bInBoundingBox = true;
        break;
      }
    }

    if (bInBoundingBox)
    {
      PlacementPoint& placementPoint = m_InputPoints.ExpandAndGetRef();
      placementPoint.m_vPosition = hitResult.m_vPosition;
      placementPoint.m_fScale = 1.0f;
      placementPoint.m_vNormal = hitResult.m_vNormal;
      placementPoint.m_uiColorIndex = 0;
      placementPoint.m_uiObjectIndex = 0;
      placementPoint.m_uiPointIndex = i;
    }
  }
}

void PlacementTask::ExecuteVM()
{
  auto pOutput = m_pData->m_pOutput;

  // Execute bytecode
  if (pOutput->m_pByteCode != nullptr)
  {
    EZ_PROFILE_SCOPE("ExecuteVM");

    ezUInt32 uiNumInstances = m_InputPoints.GetCount();
    m_TempData.SetCountUninitialized(uiNumInstances * 5);

    ezHybridArray<ezExpression::Stream, 8> inputs;
    {
      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(ezExpression::MakeStream(m_InputPoints.GetArrayPtr(), offsetof(PlacementPoint, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      // Point index
      ezArrayPtr<float> pointIndex = m_TempData.GetArrayPtr().GetSubArray(0, uiNumInstances);
      for (ezUInt32 i = 0; i < uiNumInstances; ++i)
      {
        pointIndex[i] = m_InputPoints[i].m_uiPointIndex;
      }
      inputs.PushBack(ezExpression::MakeStream(pointIndex, 0, ExpressionInputs::s_sPointIndex));
    }

    ezArrayPtr<float> density = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 1, uiNumInstances);
    ezArrayPtr<float> scale = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 2, uiNumInstances);
    ezArrayPtr<float> colorIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 3, uiNumInstances);
    ezArrayPtr<float> objectIndex = m_TempData.GetArrayPtr().GetSubArray(uiNumInstances * 4, uiNumInstances);

    ezHybridArray<ezExpression::Stream, 8> outputs;
    {
      outputs.PushBack(ezExpression::MakeStream(density, 0, ExpressionOutputs::s_sDensity));
      outputs.PushBack(ezExpression::MakeStream(scale, 0, ExpressionOutputs::s_sScale));
      outputs.PushBack(ezExpression::MakeStream(colorIndex, 0, ExpressionOutputs::s_sColorIndex));
      outputs.PushBack(ezExpression::MakeStream(objectIndex, 0, ExpressionOutputs::s_sObjectIndex));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumInstances, m_pData->m_GlobalData).Failed())
    {
      return;
    }

    // Test density against point threshold and fill remaining input point data from expression
    float fObjectCount = static_cast<float>(pOutput->m_ObjectsToPlace.GetCount());
    const Pattern* pPattern = pOutput->m_pPattern;
    for (ezUInt32 i = 0; i < uiNumInstances; ++i)
    {
      auto& inputPoint = m_InputPoints[i];
      ezUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
      float fThreshold = pPattern->m_Points[uiPointIndex].m_fThreshold;

      if (density[i] >= fThreshold)
      {
        inputPoint.m_fScale = scale[i];
        inputPoint.m_uiColorIndex = static_cast<ezUInt8>(ezMath::Clamp(colorIndex[i] * 256.0f, 0.0f, 255.0f));
        inputPoint.m_uiObjectIndex = static_cast<ezUInt8>(ezMath::Clamp(objectIndex[i] * fObjectCount, 0.0f, fObjectCount - 1.0f));

        m_ValidPoints.PushBack(i);
      }
    }
  }

  if (m_ValidPoints.IsEmpty())
  {
    return;
  }

  EZ_PROFILE_SCOPE("Construct final transforms");

  m_OutputTransforms.SetCountUninitialized(m_ValidPoints.GetCount());

  ezSimdVec4u seed = ezSimdVec4u(m_pData->m_iTileSeed) + ezSimdVec4u(0, 3, 7, 11);

  float fMinAngle = 0.0f;
  float fMaxAngle = ezMath::Pi<float>() * 2.0f;

  ezSimdVec4f vMinValue = ezSimdVec4f(fMinAngle, pOutput->m_vMinOffset.z, 0.0f);
  ezSimdVec4f vMaxValue = ezSimdVec4f(fMaxAngle, pOutput->m_vMaxOffset.z, 0.0f);
  ezSimdVec4f vUp = ezSimdVec4f(0, 0, 1);
  ezSimdVec4f vAlignToNormal = ezSimdVec4f(pOutput->m_fAlignToNormal);
  ezSimdVec4f vMinScale = ezSimdConversion::ToVec3(pOutput->m_vMinScale);
  ezSimdVec4f vMaxScale = ezSimdConversion::ToVec3(pOutput->m_vMaxScale);

  const ezColorGradient* pColorGradient = nullptr;
  if (pOutput->m_hColorGradient.IsValid())
  {
    ezResourceLock<ezColorGradientResource> pColorGradientResource(pOutput->m_hColorGradient, ezResourceAcquireMode::BlockTillLoaded);
    pColorGradient = &(pColorGradientResource->GetDescriptor().m_Gradient);
  }

  for (ezUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
  {
    ezUInt32 uiInputPointIndex = m_ValidPoints[i];
    auto& placementPoint = m_InputPoints[uiInputPointIndex];
    auto& placementTransform = m_OutputTransforms[i];

    ezSimdVec4f random = ezSimdRandom::FloatMinMax(seed + ezSimdVec4u(placementPoint.m_uiPointIndex), vMinValue, vMaxValue);

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
      float colorIndex = ezMath::ColorByteToFloat(placementPoint.m_uiColorIndex);
      ezUInt8 alpha;
      pColorGradient->EvaluateColor(colorIndex, objectColor);
      pColorGradient->EvaluateAlpha(colorIndex, alpha);
      objectColor.a = alpha;
    }
    placementTransform.m_Color = objectColor;

    placementTransform.m_uiObjectIndex = placementPoint.m_uiObjectIndex;
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
  }
}
