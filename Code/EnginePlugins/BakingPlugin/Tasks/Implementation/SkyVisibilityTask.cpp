#include <BakingPluginPCH.h>

#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tasks/Utils.h>
#include <BakingPlugin/Tracer/TracerInterface.h>

using namespace ezBakingInternal;

SkyVisibilityTask::SkyVisibilityTask(ezTracerInterface* pTracer, ezArrayPtr<const ezVec3> probePositions)
  : m_pTracer(pTracer)
  , m_ProbePositions(probePositions)
{
}

SkyVisibilityTask::~SkyVisibilityTask() = default;

void SkyVisibilityTask::Execute()
{
  m_SkyVisibility.SetCountUninitialized(m_ProbePositions.GetCount());

  const ezUInt32 uiNumSamples = 128;
  ezHybridArray<ezTracerInterface::Ray, 128> rays;
  rays.SetCountUninitialized(uiNumSamples);

  ezAmbientCube<float> weightNormalization;
  for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
  {
    auto& ray = rays[uiSampleIndex];
    ray.m_vDir = FibonacciSphere(uiSampleIndex, uiNumSamples);
    ray.m_fDistance = 1000.0f;

    weightNormalization.AddSample(ray.m_vDir, 1.0f);
  }

  for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
  {
    weightNormalization.m_Values[i] = 1.0f / weightNormalization.m_Values[i];
  }

  ezHybridArray<ezTracerInterface::Hit, 128> hits;
  hits.SetCountUninitialized(uiNumSamples);

  for (ezUInt32 uiProbeIndex = 0; uiProbeIndex < m_ProbePositions.GetCount(); ++uiProbeIndex)
  {
    ezVec3 probePos = m_ProbePositions[uiProbeIndex];
    for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      rays[uiSampleIndex].m_vStartPos = probePos;
    }

    m_pTracer->TraceRays(rays, hits);

    ezAmbientCube<float> skyVisibility;
    for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      const auto& ray = rays[uiSampleIndex];
      const auto& hit = hits[uiSampleIndex];
      const float value = hit.m_fDistance < 0.0f ? 1.0f : 0.0f;

      skyVisibility.AddSample(ray.m_vDir, value);
    }

    auto& compressedSkyVisibility = m_SkyVisibility[uiProbeIndex];
    for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
    {
      compressedSkyVisibility.m_Values[i] = ezMath::ColorFloatToByte(skyVisibility.m_Values[i] * weightNormalization.m_Values[i]);
    }
  }
}
