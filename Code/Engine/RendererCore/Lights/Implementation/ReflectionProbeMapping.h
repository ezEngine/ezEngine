#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Event generated on mapping changes.
/// \sa ezReflectionProbeMapping::m_Events
struct ezReflectionProbeMappingEvent
{
  enum class Type
  {
    ProbeMapped,          ///< The given probe was mapped to the atlas.
    ProbeUnmapped,        ///<  The given probe was unmapped from the atlas.
    ProbeUpdateRequested, ///< The given probe needs to be updated after which ezReflectionProbeMapping::ProbeUpdateFinished must be called.
  };

  ezReflectionProbeId m_Id;
  Type m_Type;
};

/// \brief This class creates a reflection probe atlas and controls the mapping of added probes to the available atlas indices.
class ezReflectionProbeMapping
{
public:
  /// \brief Creates a reflection probe atlas and mapping of the given size.
  /// \param uiAtlasSize How many probes the atlas can contain.
  ezReflectionProbeMapping(ezUInt32 uiAtlasSize);
  ~ezReflectionProbeMapping();

  /// \name Probe management
  ///@{

  /// \brief Adds a probe that will be considered for mapping into the atlas.
  void AddProbe(ezReflectionProbeId probe, ezBitflags<ezProbeFlags> flags);

  /// \brief Marks previously added probe as dirty and potentially changes its flags.
  void UpdateProbe(ezReflectionProbeId probe, ezBitflags<ezProbeFlags> flags);

  /// \brief Should be called once a requested ezReflectionProbeMappingEvent::Type::ProbeUpdateRequested event has been completed.
  /// \param probe The probe that has finished its update.
  void ProbeUpdateFinished(ezReflectionProbeId probe);

  /// \brief Removes a probe. If the probe was mapped, ezReflectionProbeMappingEvent::Type::ProbeUnmapped will be fired when calling this function.
  void RemoveProbe(ezReflectionProbeId probe);

  ///@}
  /// \name Render helpers
  ///@{

  /// \brief Returns the index at which a given probe is mapped.
  /// \param probe The probe that is being queried.
  /// \param bForExtraction If set, returns whether the index can be used for using the probe during rendering. If the probe was just mapped but not updated yet, -1 will be returned for bForExtraction = true but a valid index for bForExtraction = false so that the index can be rendered into.
  /// \return Returns the mapped index in the atlas or -1 of the probe is not mapped.
  ezInt32 GetReflectionIndex(ezReflectionProbeId probe, bool bForExtraction = false) const;

  /// \brief Returns the atlas texture.
  /// \return The texture handle of the cube map atlas.
  ezGALTextureHandle GetTexture() const { return m_hReflectionSpecularTexture; }

  ///@}
  /// \name Compute atlas mapping
  ///@{

  /// \brief Should be called in the PreExtraction phase. This will reset all probe weights.
  void PreExtraction();

  /// \brief Adds weight to a probe. Should be called during extraction of the probe. The mapping will map the probes with the highest weights in the atlas over time. This can be called multiple times in a frame for a probe if it is visible in multiple views. The maximum weight is then taken.
  void AddWeight(ezReflectionProbeId probe, float fPriority);

  /// \brief Should be called in the PostExtraction phase. This will compute the best probe mapping and potentially fire ezReflectionProbeMappingEvent events to map / unmap or request updates of probes.
  void PostExtraction();

  ///@}

public:
  ezEvent<const ezReflectionProbeMappingEvent&> m_Events;

private:
  struct ezProbeMappingFlags
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      SkyLight = ezProbeFlags::SkyLight,
      HasCustomCubeMap = ezProbeFlags::HasCustomCubeMap,
      Sphere = ezProbeFlags::Sphere,
      Box = ezProbeFlags::Box,
      Dynamic = ezProbeFlags::Dynamic,
      Dirty = EZ_BIT(5),
      Usable = EZ_BIT(6),
      Default = 0
    };

    struct Bits
    {
      StorageType SkyLight : 1;
      StorageType HasCustomCubeMap : 1;
      StorageType Sphere : 1;
      StorageType Box : 1;
      StorageType Dynamic : 1;
      StorageType Dirty : 1;
      StorageType Usable : 1;
    };
  };

  // EZ_DECLARE_FLAGS_OPERATORS(ezProbeMappingFlags);

  struct SortedProbes
  {
    EZ_DECLARE_POD_TYPE();

    EZ_ALWAYS_INLINE bool operator<(const SortedProbes& other) const
    {
      if (m_fPriority > other.m_fPriority) // we want to sort descending (higher priority first)
        return true;

      return m_uiIndex < other.m_uiIndex;
    }

    ezReflectionProbeId m_uiIndex;
    float m_fPriority = 0.0f;
  };

  struct ProbeDataInternal
  {
    ezBitflags<ezProbeMappingFlags> m_Flags;
    ezInt32 m_uiReflectionIndex = -1;
    float m_fPriority = 0.0f;
    ezReflectionProbeId m_id;
  };

private:
  void MapProbe(ezReflectionProbeId id, ezInt32 iReflectionIndex);
  void UnmapProbe(ezReflectionProbeId id);

private:
  ezDynamicArray<ProbeDataInternal> m_RegisteredProbes;
  ezReflectionProbeId m_SkyLight;

  ezUInt32 m_uiAtlasSize = 32;
  ezDynamicArray<ezReflectionProbeId> m_MappedCubes;

  // GPU Data
  ezGALTextureHandle m_hReflectionSpecularTexture;

  // Cleared every frame:
  ezDynamicArray<SortedProbes> m_SortedProbes; // All probes exiting in the scene, sorted by priority.
  ezDynamicArray<SortedProbes> m_ActiveProbes; // Probes that are currently mapped in the atlas.
  ezDynamicArray<ezInt32> m_UnusedProbeSlots;  // Probe slots are are currently unused in the atlas.
  ezDynamicArray<SortedProbes> m_AddProbes;    // Probes that should be added to the atlas
};
