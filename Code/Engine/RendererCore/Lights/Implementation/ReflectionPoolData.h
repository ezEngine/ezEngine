#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/Pipeline/View.h>

class ezSkyLightComponent;
class ezSphereReflectionProbeComponent;
class ezBoxReflectionProbeComponent;

static const ezUInt32 s_uiReflectionCubeMapSize = 128;
static const ezUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static const float s_fDebugSphereRadius = 0.3f;

inline ezUInt32 GetMipLevels()
{
  return ezMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
}

//////////////////////////////////////////////////////////////////////////
/// ezReflectionPool::Data

struct ezReflectionPool::Data
{
  Data();
  ~Data();

  struct ProbeData
  {
    ezReflectionProbeDesc m_desc;
    ezTransform m_GlobalTransform;
    ezBitflags<ezProbeFlags> m_Flags;
    ezTextureCubeResourceHandle m_hCubeMap; // static data or empty for dynamic.
  };

  struct WorldReflectionData
  {
    WorldReflectionData()
      : m_mapping(s_uiNumReflectionProbeCubeMaps)
    {
    }
    EZ_DISALLOW_COPY_AND_ASSIGN(WorldReflectionData);

    ezIdTable<ezReflectionProbeId, ProbeData> m_Probes;
    ezReflectionProbeId m_SkyLight; // SkyLight is always fixed at reflectionIndex 0.
    ezEventSubscriptionID m_mappingSubscriptionId = 0;
    ezReflectionProbeMapping m_mapping;
  };

  // WorldReflectionData management
  ezReflectionProbeId AddProbe(const ezWorld* pWorld, ProbeData&& probeData);
  ezReflectionPool::Data::WorldReflectionData& GetWorldData(const ezWorld* pWorld);
  void RemoveProbe(const ezWorld* pWorld, ezReflectionProbeId id);
  void UpdateProbeData(ProbeData& ref_probeData, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent);
  bool UpdateSkyLightData(ProbeData& ref_probeData, const ezReflectionProbeDesc& desc, const ezSkyLightComponent* pComponent);
  void OnReflectionProbeMappingEvent(const ezUInt32 uiWorldIndex, const ezReflectionProbeMappingEvent& e);

  void PreExtraction();
  void PostExtraction();

  // Dynamic Update Queue (all worlds combined)
  ezHashSet<ezReflectionProbeRef> m_PendingDynamicUpdate;
  ezDeque<ezReflectionProbeRef> m_DynamicUpdateQueue;

  ezHashSet<ezReflectionProbeRef> m_ActiveDynamicUpdate;
  ezReflectionProbeUpdater m_ReflectionProbeUpdater;

  void CreateReflectionViewsAndResources();
  void CreateSkyIrradianceTexture();

  ezMutex m_Mutex;
  ezUInt64 m_uiWorldHasSkyLight = 0;
  ezUInt64 m_uiSkyIrradianceChanged = 0;
  ezHybridArray<ezUniquePtr<WorldReflectionData>, 2> m_WorldReflectionData;

  // GPU storage
  ezGALTextureHandle m_hFallbackReflectionSpecularTexture;
  ezGALTextureHandle m_hSkyIrradianceTexture;
  ezHybridArray<ezAmbientCube<ezColorLinear16f>, 64> m_SkyIrradianceStorage;

  // Debug data
  ezMeshResourceHandle m_hDebugSphere;
  ezHybridArray<ezMaterialResourceHandle, 6 * s_uiNumReflectionProbeCubeMaps> m_hDebugMaterial;
};
