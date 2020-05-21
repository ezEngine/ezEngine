#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInterface.h>
#include <Core/World/SettingsComponentManager.h>

//////////////////////////////////////////////////////////////////////////

typedef ezSettingsComponentManager<class ezStageSpaceComponent> ezStageSpaceComponentManager;

/// \brief Singleton to set the type of stage space and its global transform in the world.
///
/// The global transform of the owner and the set stage space are read out by the XR
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

  /// \brief Sets the stage space used by the XR experience.
  void SetStageSpace(ezEnum<ezXRStageSpace> space);
  ezEnum<ezXRStageSpace> GetStageSpace() const;

protected:
  //
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

private:
  ezEnum<ezXRStageSpace> m_space;
};

