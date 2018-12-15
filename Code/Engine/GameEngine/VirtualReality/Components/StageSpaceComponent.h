#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <Core/World/SettingsComponentManager.h>

//////////////////////////////////////////////////////////////////////////

typedef ezSettingsComponentManager<class ezStageSpaceComponent> ezStageSpaceComponentManager;

/// \brief Singleton to set the type of stage space and its global transform in the world.
///
/// The global transform of the owner and the set stage space are read out by the VR
/// implementation every frame.
class EZ_GAMEENGINE_DLL ezStageSpaceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezStageSpaceComponent, ezComponent, ezStageSpaceComponentManager);

public:
  ezStageSpaceComponent();
  ~ezStageSpaceComponent();

  //
  // ezDeviceTrackingComponent Interface
  //

  /// \brief Sets the stage space used by the VR experience.
  void SetStageSpace(ezEnum<ezVRStageSpace> space);
  ezEnum<ezVRStageSpace> GetStageSpace() const;

protected:
  //
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

private:
  ezEnum<ezVRStageSpace> m_space;
};

