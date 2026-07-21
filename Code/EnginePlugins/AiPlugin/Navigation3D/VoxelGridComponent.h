#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/SpatialData.h>

using ezAiVoxelGridComponentManager = ezComponentManager<class ezAiVoxelGridComponent, ezBlockStorageType::Compact>;

/// Drop this component into a scene to add a 3D voxel navigation grid.
///
/// The grid is centered on this component's game object position.
class EZ_AIPLUGIN_DLL ezAiVoxelGridComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiVoxelGridComponent, ezComponent, ezAiVoxelGridComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiVoxelGridComponent

public:
  ezAiVoxelGridComponent();
  ~ezAiVoxelGridComponent();

  const ezVec3& GetSize() const { return m_vSize; }
  void SetSize(const ezVec3& vSize);

  float GetVoxelSize() const { return m_fVoxelSize; }               // [ property ]
  void SetVoxelSize(float fValue);                                  // [ property ]

  ezUInt32 GetCollisionLayer() const { return m_uiCollisionLayer; } // [ property ]
  void SetCollisionLayer(ezUInt32 uiValue);                         // [ property ]

  /// If true, or if the AI.VoxelGrid.Visualize CVar is set, the grid is drawn using the debug renderer.
  bool m_bVisualize = false; // [ property ]

  void VoxelizeWorld();

  const ezVoxelGrid& GetStaticVoxelGrid() const { return m_StaticGrid; }

  /// Spatial data category used to register grid components with the world's spatial system.
  ///
  /// Used by ezAiVoxelWorldModule::FindGridsInBox() to efficiently query grid components in an area.
  static ezSpatialData::Category SpatialDataCategory;

protected:
  void OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]

private:
  ezVec3 m_vSize = ezVec3(32.0f);
  float m_fVoxelSize = 0.5f;
  ezUInt32 m_uiCollisionLayer = 0;

  ezVoxelGrid m_StaticGrid;

  bool m_bNeedsVoxelization = true;
};
