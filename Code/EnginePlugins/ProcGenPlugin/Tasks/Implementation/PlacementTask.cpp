#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <RendererCore/Debug/DebugRenderer.h>

using namespace ezProcGenInternal;

static_assert(sizeof(PlacementPoint) == 32);
static_assert(sizeof(PlacementTransform) == 64);

PlacementTask::PlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, ezTaskNesting::Maybe);

  m_VM.RegisterFunction(ezProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(ezProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

PlacementTask::~PlacementTask() = default;

void PlacementTask::Clear()
{
  m_InputPoints.Clear();
  m_OutputTransforms.Clear();
  m_Density.Clear();
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

bool IsRequestedSurface(ezSurfaceResourceHandle hRequestedSurface, ezSurfaceResourceHandle hHitSurface)
{
  if (hRequestedSurface.IsValid())
  {
    if (!hHitSurface.IsValid())
      return false;

    ezResourceLock<ezSurfaceResource> hitSurface(hHitSurface, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (hitSurface.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
      return false;

    if (!hitSurface->IsBasedOn(hRequestedSurface))
      return false;
  }

  return true;
}

void PlacementTask::FindPlacementPoints()
{
  EZ_PROFILE_SCOPE("FindPlacementPoints");

  auto pOutput = m_pData->m_pOutput;

  ezSimdVec4u seed = ezSimdVec4u(m_pData->m_uiTileSeed) + ezSimdVec4u(0, 3, 7, 11);

  float fZRange = m_pData->m_TileBoundingBox.GetExtents().z;
  ezSimdFloat fZStart = m_pData->m_TileBoundingBox.m_vMax.z;
  ezSimdVec4f vXY = ezSimdConversion::ToVec3(m_pData->m_TileBoundingBox.m_vMin);
  ezSimdVec4f vMinOffset = ezSimdConversion::ToVec3(pOutput->m_vMinOffset);
  ezSimdVec4f vMaxOffset = ezSimdConversion::ToVec3(pOutput->m_vMaxOffset);

  // use center for fixed plane placement
  vXY.SetZ(m_pData->m_TileBoundingBox.GetCenter().z);

  ezVec3 rayDir = ezVec3(0, 0, -1);
  ezUInt32 uiCollisionLayer = pOutput->m_uiCollisionLayer;

  auto& patternPoints = pOutput->m_pPattern->m_Points;

  for (ezUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    auto& patternPoint = patternPoints[i];
    ezSimdVec4f patternCoords = ezSimdVec4f(patternPoint.x, patternPoint.y, 0.0f);

    ezPhysicsCastResult hitResult;

    if (m_pData->m_pPhysicsModule != nullptr &&
        (pOutput->m_Mode == ezProcPlacementMode::Raycast ||
          pOutput->m_Mode == ezProcPlacementMode::RaycastHighQuality))
    {
      ezSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += ezSimdRandom::FloatMinMax(ezSimdVec4i(i), vMinOffset, vMaxOffset, seed);
      rayStart.SetZ(fZStart);

      ezPhysicsQueryParameters queryParams(uiCollisionLayer, ezPhysicsShapeType::Static);

      if (!m_pData->m_pPhysicsModule->Raycast(hitResult, ezSimdConversion::ToVec3(rayStart), rayDir, fZRange, queryParams))
        continue;

      if (!IsRequestedSurface(pOutput->m_hSurface, hitResult.m_hSurface))
        continue;

      if (pOutput->m_Mode == ezProcPlacementMode::RaycastHighQuality)
      {
        const ezUInt32 uiNumAdditionalRays = ezMath::Max<ezUInt32>(pOutput->m_uiNumAdditionalRays, 3);
        const ezAngle angleStep = ezAngle::MakeFromDegree(360.0f / uiNumAdditionalRays);
        const float fSpread = ezMath::Max(pOutput->m_fRaySpread * pOutput->m_fFootprint, 0.01f);

        ezHybridArray<ezVec3, 32> hitPositions;

        bool bAllValid = true;
        for (ezUInt32 i = 0; i < uiNumAdditionalRays; ++i)
        {
          const ezAngle angle = angleStep * float(i);
          const ezSimdVec4f offset = ezSimdVec4f(ezMath::Cos(angle), ezMath::Sin(angle), 0.0f) * fSpread;
          const ezSimdVec4f rayStartOffset = rayStart + offset;

          ezPhysicsCastResult offsetHitResult;
          if (!m_pData->m_pPhysicsModule->Raycast(offsetHitResult, ezSimdConversion::ToVec3(rayStartOffset), rayDir, fZRange, queryParams))
          {
            bAllValid = false;
            break;
          }

          if (!IsRequestedSurface(pOutput->m_hSurface, offsetHitResult.m_hSurface))
          {
            bAllValid = false;
            break;
          }

          hitPositions.PushBack(offsetHitResult.m_vPosition);
        }

        if (!bAllValid)
          continue;

        const ezSimdVec4f up = ezSimdVec4f(0, 0, 1);
        ezSimdVec4f avgNormal = ezSimdVec4f::MakeZero();
        for (auto& pos : hitPositions)
        {
          // Do not normalize dirToP, so that points that are further away contribute more to the normal
          const ezSimdVec4f dirToP = (ezSimdConversion::ToVec3(hitResult.m_vPosition) - ezSimdConversion::ToVec3(pos));
          const ezSimdVec4f rightDir = up.CrossRH(dirToP);
          const ezSimdVec4f normal = dirToP.CrossRH(rightDir);
          avgNormal += normal;
        }

        hitResult.m_vNormal = ezSimdConversion::ToVec3(avgNormal.GetNormalized<3>());
      }
    }
    else if (pOutput->m_Mode == ezProcPlacementMode::Fixed)
    {
      ezSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += ezSimdRandom::FloatMinMax(ezSimdVec4i(i), vMinOffset, vMaxOffset, seed);

      hitResult.m_vPosition = ezSimdConversion::ToVec3(rayStart);
      hitResult.m_fDistance = 0;
      hitResult.m_vNormal.Set(0, 0, 1);
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
      placementPoint.m_uiPointIndex = static_cast<ezUInt16>(i);
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
    m_Density.SetCountUninitialized(uiNumInstances);

    ezHybridArray<ezProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionX, offsetof(PlacementPoint, m_vPosition.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionY, offsetof(PlacementPoint, m_vPosition.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionZ, offsetof(PlacementPoint, m_vPosition.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalX, offsetof(PlacementPoint, m_vNormal.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalY, offsetof(PlacementPoint, m_vNormal.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalZ, offsetof(PlacementPoint, m_vNormal.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPointIndex, offsetof(PlacementPoint, m_uiPointIndex), ezProcessingStream::DataType::Short));
    }

    ezHybridArray<ezProcessingStream, 8> outputs;
    {
      outputs.PushBack(ezProcessingStream(ExpressionOutputs::s_sOutDensity, m_Density.GetByteArrayPtr(), ezProcessingStream::DataType::Float));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutScale, offsetof(PlacementPoint, m_fScale)));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutColorIndex, offsetof(PlacementPoint, m_uiColorIndex), ezProcessingStream::DataType::Byte));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutObjectIndex, offsetof(PlacementPoint, m_uiObjectIndex), ezProcessingStream::DataType::Byte));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumInstances, m_pData->m_GlobalData, ezExpressionVM::Flags::BestPerformance).Failed())
    {
      return;
    }

    // Test density against point threshold and fill remaining input point data from expression
    const Pattern* pPattern = pOutput->m_pPattern;
    for (ezUInt32 i = 0; i < uiNumInstances; ++i)
    {
      auto& inputPoint = m_InputPoints[i];
      const ezUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
      const float fThreshold = pPattern->m_Points[uiPointIndex].threshold;

      if (m_Density[i] >= fThreshold)
      {
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

  ezSimdVec4u seed = ezSimdVec4u(m_pData->m_uiTileSeed) + ezSimdVec4u(13, 17, 31, 79);

  float fMinAngle = 0.0f;
  float fMaxAngle = ezMath::Pi<float>() * 2.0f;

  ezSimdVec4f vMinValue = ezSimdVec4f(fMinAngle, pOutput->m_vMinOffset.z, 0.0f);
  ezSimdVec4f vMaxValue = ezSimdVec4f(fMaxAngle, pOutput->m_vMaxOffset.z, 0.0f);
  ezSimdVec4f vYawRotationSnap = ezSimdVec4f(pOutput->m_YawRotationSnap);
  ezSimdVec4f vUp = ezSimdVec4f(0, 0, 1);
  ezSimdVec4f vHalf = ezSimdVec4f(0.5f);
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

    ezSimdVec4f random = ezSimdRandom::FloatMinMax(ezSimdVec4i(placementPoint.m_uiPointIndex), vMinValue, vMaxValue, seed);

    ezSimdVec4f offset = ezSimdVec4f::MakeZero();
    offset.SetZ(random.y());
    placementTransform.m_Transform.m_Position = ezSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;

    ezSimdVec4f yaw = ezSimdVec4f(random.x());
    ezSimdVec4f roundedYaw = (yaw.CompDiv(vYawRotationSnap) + vHalf).Floor().CompMul(vYawRotationSnap);
    yaw = ezSimdVec4f::Select(vYawRotationSnap == ezSimdVec4f::MakeZero(), yaw, roundedYaw);

    ezSimdQuat qYawRot = ezSimdQuat::MakeFromAxisAndAngle(vUp, yaw.x());
    ezSimdVec4f vNormal = ezSimdConversion::ToVec3(placementPoint.m_vNormal);
    ezSimdQuat qToNormalRot = ezSimdQuat::MakeShortestRotation(vUp, ezSimdVec4f::Lerp(vUp, vNormal, vAlignToNormal));
    placementTransform.m_Transform.m_Rotation = qToNormalRot * qYawRot;

    ezSimdVec4f scale = ezSimdVec4f(ezMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));
    placementTransform.m_Transform.m_Scale = ezSimdVec4f::Lerp(vMinScale, vMaxScale, scale);

    placementTransform.m_ObjectColor = ezColor::MakeZero();
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
    placementTransform.m_uiObjectIndex = placementPoint.m_uiObjectIndex;
    placementTransform.m_bHasValidColor = false;

    if (pColorGradient != nullptr)
    {
      float colorIndex = ezMath::ColorByteToFloat(placementPoint.m_uiColorIndex);

      ezColor objectColor;
      ezUInt8 alpha;
      float intensity = 1.0f;
      pColorGradient->EvaluateColor(colorIndex, objectColor);
      pColorGradient->EvaluateIntensity(colorIndex, intensity);
      pColorGradient->EvaluateAlpha(colorIndex, alpha);
      objectColor.r *= intensity;
      objectColor.g *= intensity;
      objectColor.b *= intensity;
      objectColor.a = alpha;

      placementTransform.m_ObjectColor = objectColor;
      placementTransform.m_bHasValidColor = true;
    }
  }
}
