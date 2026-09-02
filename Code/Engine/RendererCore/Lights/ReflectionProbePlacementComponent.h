#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgUpdateLocalBounds;

/// Which kind of space reflection probes should be placed in.
struct EZ_RENDERERCORE_DLL ezReflectionProbePlacementMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Indoors,  ///< Only where something is overhead. Open areas are left to the sky light.
    Outdoors, ///< Only where the sky is visible. Probes are spread far apart to cover large areas.
    Both,     ///< Everywhere that is walkable.

    Default = Both
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezReflectionProbePlacementMode);

using ezReflectionProbePlacementComponentManager = ezComponentManager<class ezReflectionProbePlacementComponent, ezBlockStorageType::Compact>;

/// Marks a volume in which reflection probes should be placed automatically.
///
/// This component does nothing at runtime. It only provides the region and the settings for the
/// "Place Reflection Probes" long op, which is triggered through a button in the editor's property grid.
/// The placement analyzes the scene geometry inside the volume by building navmeshes, and then adds
/// probe components to the scene. The generated objects are placed under a dedicated child object and
/// are fully editable afterwards. Running the operation again replaces the previously generated probes.
///
/// Since the analysis is based on navmeshes, probes are only placed where a character could walk.
/// Areas that can't be reached (for instance the upper part of a tall hall) don't get probes.
class EZ_RENDERERCORE_DLL ezReflectionProbePlacementComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezReflectionProbePlacementComponent, ezComponent, ezReflectionProbePlacementComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezReflectionProbePlacementComponent

public:
  ezReflectionProbePlacementComponent();
  ~ezReflectionProbePlacementComponent();

  const ezVec3& GetExtents() const { return m_vExtents; }   // [ property ]
  void SetExtents(const ezVec3& vExtents);                  // [ property ]

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const;

  /// Size of the voxels used to analyze the scene. Larger values are much faster but less precise.
  float m_fCellSize = 0.5f;                                 // [ property ]

  /// Height above the ground at which probes are placed. Should roughly match the eye height of the player.
  float m_fProbeHeight = 1.7f;                              // [ property ]

  /// Walkable areas smaller than this (in square meters) don't get their own probe.
  ///
  /// Small areas are merged into a neighboring probe instead, if there is one close enough.
  float m_fMinAreaSize = 4.0f;                              // [ property ]

  /// Radius of the largest character that is used to detect wide open areas.
  ///
  /// A second analysis pass uses this radius. Areas that survive it are considered large rooms and get
  /// bigger probes. Set this to 0 to disable the second pass.
  float m_fLargeAreaRadius = 1.5f;                          // [ property ]

  /// Probes closer to each other than this are merged into a single probe.
  float m_fMergeDistance = 4.0f;                            // [ property ]

  /// Upper limit for how many probes may be generated.
  ezUInt32 m_uiMaxProbes = 64;                              // [ property ]

  /// Largest radius or half extent a generated probe may have.
  ///
  /// A probe captures its surroundings from a single point, so a very large one ends up reflecting
  /// geometry that is nowhere near the surfaces it is applied to.
  float m_fMaxProbeSize = 12.0f;                            // [ property ]

  /// Spots with more head room than this count as outdoors rather than indoors.
  ///
  /// This also determines how tall a generated box probe may become.
  float m_fMaxCeilingHeight = 8.0f;                         // [ property ]

  /// Roughly how far apart probes are placed outdoors.
  ///
  /// Outdoors there is little nearby geometry to reflect, so a few widely spaced probes look just as
  /// good as many close ones and cost far less. Probes are sized to reach their neighbours.
  float m_fOutdoorSpacing = 30.0f;                          // [ property ]

  /// Whether probes are placed in enclosed spaces, in the open, or both.
  ezEnum<ezReflectionProbePlacementMode> m_Mode;            // [ property ]

  /// Whether to generate box probes for areas that are enclosed by walls.
  ///
  /// Box probes use box projection, which looks much better in rectangular rooms, but wrong in open spaces.
  /// If disabled, only sphere probes are generated.
  bool m_bGenerateBoxProbes = true;                         // [ property ]

private:
  ezVec3 m_vExtents = ezVec3(50.0f);
};
