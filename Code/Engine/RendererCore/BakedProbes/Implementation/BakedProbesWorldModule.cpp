#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezBakedProbesWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBakedProbesWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezBakedProbesWorldModule::ezBakedProbesWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

ezBakedProbesWorldModule::~ezBakedProbesWorldModule() = default;

void ezBakedProbesWorldModule::Initialize()
{
}

void ezBakedProbesWorldModule::Deinitialize()
{
}

bool ezBakedProbesWorldModule::HasProbeData() const
{
  return m_hProbeTree.IsValid();
}

ezResult ezBakedProbesWorldModule::GetProbeIndexData(const ezVec3& vGlobalPosition, const ezVec3& vNormal, ProbeIndexData& out_probeIndexData) const
{
  // TODO: optimize

  if (!HasProbeData())
    return EZ_FAILURE;

  ezResourceLock<ezProbeTreeSectorResource> pProbeTree(m_hProbeTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  ezSimdVec4f gridSpacePos = ezSimdConversion::ToVec3((vGlobalPosition - pProbeTree->GetGridOrigin()).CompDiv(pProbeTree->GetProbeSpacing()));
  gridSpacePos = gridSpacePos.CompMax(ezSimdVec4f::MakeZero());

  ezSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  ezSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  ezSimdVec4i maxIndices = ezSimdVec4i(pProbeTree->GetProbeCount().x, pProbeTree->GetProbeCount().y, pProbeTree->GetProbeCount().z) - ezSimdVec4i(1);
  ezSimdVec4i pos0 = ezSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  ezSimdVec4i pos1 = (pos0 + ezSimdVec4i(1)).CompMin(maxIndices);

  ezUInt32 x0 = pos0.x();
  ezUInt32 y0 = pos0.y();
  ezUInt32 z0 = pos0.z();

  ezUInt32 x1 = pos1.x();
  ezUInt32 y1 = pos1.y();
  ezUInt32 z1 = pos1.z();

  ezUInt32 xCount = pProbeTree->GetProbeCount().x;
  ezUInt32 xyCount = xCount * pProbeTree->GetProbeCount().y;

  out_probeIndexData.m_probeIndices[0] = z0 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[1] = z0 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[2] = z0 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[3] = z0 * xyCount + y1 * xCount + x1;
  out_probeIndexData.m_probeIndices[4] = z1 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[5] = z1 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[6] = z1 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[7] = z1 * xyCount + y1 * xCount + x1;

  ezVec3 w1 = ezSimdConversion::ToVec3(weights);
  ezVec3 w0 = ezVec3(1.0f) - w1;

  // TODO: add geometry factor to weight
  out_probeIndexData.m_probeWeights[0] = w0.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[1] = w1.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[2] = w0.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[3] = w1.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[4] = w0.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[5] = w1.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[6] = w0.x * w1.y * w1.z;
  out_probeIndexData.m_probeWeights[7] = w1.x * w1.y * w1.z;

  float weightSum = 0;
  for (ezUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    weightSum += out_probeIndexData.m_probeWeights[i];
  }

  float normalizeFactor = 1.0f / weightSum;
  for (ezUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    out_probeIndexData.m_probeWeights[i] *= normalizeFactor;
  }

  return EZ_SUCCESS;
}

ezAmbientCube<float> ezBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  // TODO: optimize

  ezAmbientCube<float> result;

  ezResourceLock<ezProbeTreeSectorResource> pProbeTree(m_hProbeTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != ezResourceAcquireResult::Final)
    return result;

  auto compressedSkyVisibility = pProbeTree->GetSkyVisibility();
  ezAmbientCube<float> skyVisibility;

  for (ezUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    ezBakingUtils::DecompressSkyVisibility(compressedSkyVisibility[indexData.m_probeIndices[i]], skyVisibility);

    for (ezUInt32 d = 0; d < ezAmbientCubeBasis::NumDirs; ++d)
    {
      result.m_Values[d] += skyVisibility.m_Values[d] * indexData.m_probeWeights[i];
    }
  }

  return result;
}

void ezBakedProbesWorldModule::SetProbeTreeResourcePrefix(const ezHashedString& prefix)
{
  ezStringBuilder sResourcePath;
  sResourcePath.SetFormat("{}_Global.ezProbeTreeSector", prefix);

  m_hProbeTree = ezResourceManager::LoadResource<ezProbeTreeSectorResource>(sResourcePath);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
