#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>

using ezAiVoxelGridSettingsComponentManager = ezSettingsComponentManager<class ezAiVoxelGridSettingsComponent>;

/// Drop this component into a scene to configure the 3D voxel navigation grid.
///
/// Only one instance should exist per scene. It controls the grid resolution,
/// voxel size, and which collision layer to use for voxelization.
/// The grid is centered on this component's game object position.
class EZ_AIPLUGIN_DLL ezAiVoxelGridSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiVoxelGridSettingsComponent, ezSettingsComponent, ezAiVoxelGridSettingsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiVoxelGridSettingsComponent

public:
  ezAiVoxelGridSettingsComponent();
  ~ezAiVoxelGridSettingsComponent();

  ezUInt32 GetResolutionX() const { return m_uiResolutionX; } // [ property ]
  void SetResolutionX(ezUInt32 uiValue);                      // [ property ]

  ezUInt32 GetResolutionY() const { return m_uiResolutionY; } // [ property ]
  void SetResolutionY(ezUInt32 uiValue);                      // [ property ]

  ezUInt32 GetResolutionZ() const { return m_uiResolutionZ; } // [ property ]
  void SetResolutionZ(ezUInt32 uiValue);                      // [ property ]

  float GetVoxelSize() const { return m_fVoxelSize; } // [ property ]
  void SetVoxelSize(float fValue);                     // [ property ]

  ezUInt32 GetCollisionLayer() const { return m_uiCollisionLayer; } // [ property ]
  void SetCollisionLayer(ezUInt32 uiValue);                         // [ property ]

private:
  ezUInt32 m_uiResolutionX = 64;
  ezUInt32 m_uiResolutionY = 64;
  ezUInt32 m_uiResolutionZ = 32;
  float m_fVoxelSize = 0.5f;
  ezUInt32 m_uiCollisionLayer = 0;
};
