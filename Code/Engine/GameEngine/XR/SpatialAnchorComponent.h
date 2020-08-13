#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezSpatialAnchorComponent, ezComponentUpdateType::WhenSimulating> ezSpatialAnchorComponentManager;

class EZ_GAMEENGINE_DLL ezSpatialAnchorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpatialAnchorComponent, ezComponent, ezSpatialAnchorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSpatialAnchorComponent

public:
  ezSpatialAnchorComponent();
  ~ezSpatialAnchorComponent();

  /// \brief Attempts to create a new anchor at the given location.
  ///
  /// On failure, the existing anchor will continue to be used.
  /// On success, the new anchor will be used and the new location.
  ezResult RecreateAnchorAt(const ezTransform& position);

protected:
  void Update();

private:
  ezXRSpatialAnchorID m_AnchorID;
};
