#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using ezSimpleWindComponentManager = ezComponentManagerSimple<class ezSimpleWindComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Calculates one global wind force using a very basic formula.
///
/// This component computes a wind vector that varies between a minimum and maximum strength
/// and around a certain direction.
///
/// Sets up the ezSimpleWindWorldModule as the implementation of the ezWindWorldModuleInterface.
///
/// When sampling the wind through this interface, the returned value is the same at every location.
///
/// Use a single instance of this component in a scene, when you need wind values, e.g. to make cloth and ropes sway,
/// but don't need a complex wind simulation.
class EZ_GAMEENGINE_DLL ezSimpleWindComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleWindComponent, ezComponent, ezSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSimpleWindComponent

public:
  ezSimpleWindComponent();
  ~ezSimpleWindComponent();

  /// The minimum speed that the wind should always blow with.
  ezEnum<ezWindStrength> m_MinWindStrength; // [ property ]

  /// The maximum speed that the wind should blow with.
  ezEnum<ezWindStrength> m_MaxWindStrength; // [ property ]

  /// The wind blows in the positive X direction of the game object.
  /// The direction may deviate this much from that direction. Set to 180 degree to remove the limit.
  ezAngle m_Deviation; // [ property ]

protected:
  void Update();
  void ComputeNextState();

  float m_fLastStrength = 0;
  float m_fNextStrength = 0;
  ezVec3 m_vLastDirection;
  ezVec3 m_vNextDirection;
  ezTime m_LastChange;
  ezTime m_NextChange;
};
