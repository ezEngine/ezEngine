#include <BakingPluginPCH.h>

#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerInterface.h>

SkyVisibilityTask::SkyVisibilityTask(const ezBakingSettings& settings, ezTracerInterface& tracer, ezArrayPtr<const ezVec3> probePositions)
  : m_Settings(settings)
  , m_Tracer(tracer)
  , m_ProbePositions(probePositions)
{
}

SkyVisibilityTask::~SkyVisibilityTask() = default;

void SkyVisibilityTask::Execute()
{
  m_SkyVisibility.SetCountUninitialized(m_ProbePositions.GetCount());

  const ezUInt32 uiNumSamples = m_Settings.m_uiNumSamplesPerProbe;
  ezHybridArray<ezTracerInterface::Ray, 128> rays(ezFrameAllocator::GetCurrentAllocator());
  rays.SetCountUninitialized(uiNumSamples);

  ezAmbientCube<float> weightNormalization;
  for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
  {
    auto& ray = rays[uiSampleIndex];
    ray.m_vDir = ezBakingUtils::FibonacciSphere(uiSampleIndex, uiNumSamples);
    ray.m_fDistance = m_Settings.m_fMaxRayDistance;

    weightNormalization.AddSample(ray.m_vDir, 1.0f);
  }

  for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
  {
    weightNormalization.m_Values[i] = 1.0f / weightNormalization.m_Values[i];
  }

  ezHybridArray<ezTracerInterface::Hit, 128> hits(ezFrameAllocator::GetCurrentAllocator());
  hits.SetCountUninitialized(uiNumSamples);

  for (ezUInt32 uiProbeIndex = 0; uiProbeIndex < m_ProbePositions.GetCount(); ++uiProbeIndex)
  {
    ezVec3 probePos = m_ProbePositions[uiProbeIndex];
    for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      rays[uiSampleIndex].m_vStartPos = probePos;
    }

    m_Tracer.TraceRays(rays, hits);

    ezAmbientCube<float> skyVisibility;
    for (ezUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      const auto& ray = rays[uiSampleIndex];
      const auto& hit = hits[uiSampleIndex];
      const float value = hit.m_fDistance < 0.0f ? 1.0f : 0.0f;

      skyVisibility.AddSample(ray.m_vDir, value);
    }

    for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
    {
      skyVisibility.m_Values[i] *= weightNormalization.m_Values[i];
    }
    auto& compressedSkyVisibility = m_SkyVisibility[uiProbeIndex];
    compressedSkyVisibility = ezBakingUtils::CompressSkyVisibility(skyVisibility);
  }
}
