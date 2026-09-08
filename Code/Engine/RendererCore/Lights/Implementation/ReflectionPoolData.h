#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/Pipeline/View.h>

class ezSkyLightComponent;
class ezSphereReflectionProbeComponent;
class ezBoxReflectionProbeComponent;
class ezRenderGraph;

static constexpr ezUInt32 s_uiReflectionCubeMapSize = 128;
static constexpr ezUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static constexpr float s_fDebugSphereRadius = 0.3f;

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
    ezInstanceDataOffset m_DebugInstanceDataOffset;
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

  // Update Queues (all worlds combined)

  struct QueuedUpdate
  {
    EZ_DECLARE_POD_TYPE();

    ezReflectionProbeRef m_probe;
    float m_fPriority = 0.0f;
    ezUInt64 m_uiEnqueuedFrame = 0; ///< Only used by m_RefreshQueue, to age waiting probes.
  };

  /// Removes a probe from one of the update queues, if it is in there.
  static void RemoveFromQueue(ezDynamicArray<QueuedUpdate>& ref_queue, const ezReflectionProbeRef& probe);

  /// Sorts a queue so that the probe to update next is at the front.
  /// \param fAgeWeight How much a probe's priority grows per frame that it has been waiting. Zero sorts
  ///   purely by priority, which can starve low priority probes.
  static void SortQueue(ezDynamicArray<QueuedUpdate>& ref_queue, float fAgeWeight);

  // Probes that have never completed an update and thus have no usable content yet. Sorted by priority and
  // drained before m_RefreshQueue, so that a probe cannot be starved by probes that already look correct.
  ezDynamicArray<QueuedUpdate> m_InitialBakeQueue;

  // Probes that already have content and want to re-render it. Ordered by priority as well, so that the
  // probes around the camera are corrected first after a sky light update dirtied everything. The priority
  // is raised the longer a probe waits, so that distant probes cannot be starved by closer ones.
  ezDynamicArray<QueuedUpdate> m_RefreshQueue;

  // Dedup across both queues. A probe is in at most one of them.
  ezHashSet<ezReflectionProbeRef> m_PendingDynamicUpdate;

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
  ezMaterialResourceHandle m_hDebugMaterial;

  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
};
