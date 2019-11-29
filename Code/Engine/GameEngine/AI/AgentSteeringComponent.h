#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Communication/Event.h>
#include <GameEngine/GameEngineDLL.h>

struct ezAgentSteeringEvent
{
  enum Type
  {
    TargetReached,              ///< The agent reached the current target location
    TargetCleared,              ///< The agent's target location was cleared and it is now not moving further
    PathToTargetFound,          ///< Path-finding was successful, agent will follow the path now.
    ErrorInvalidTargetPosition, ///< The target position cannot be reached because it is not inside the navigation area
    ErrorNoPathToTarget,        ///< Path-finding failed, the target location cannot be reached.
    WarningNoFullPathToTarget,  ///< Path-finding resulted in a partial path, so one can get closer to it, but the target cannot be reached.
    ErrorOutsideNavArea,        ///< The current agent position is outside valid navigation area
    ErrorSteeringFailed,        ///< Some generic error
  };

  Type m_Type;
  class ezAgentSteeringComponent* m_pComponent = nullptr;
};

struct ezAgentPathFindingState
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    HasNoTarget,
    HasTargetWaitingForPath,
    HasTargetPathFindingFailed,
    HasTargetAndValidPath,

    Default = HasNoTarget
  };
};

/// \brief Base class for components that implement 'agent steering' behavior.
/// If, moving from point A to point B using something like a navmesh.
class EZ_GAMEENGINE_DLL ezAgentSteeringComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezAgentSteeringComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAgentSteeringComponent

public:
  ezAgentSteeringComponent();
  ~ezAgentSteeringComponent();

  virtual void SetTargetPosition(const ezVec3& vPosition) = 0; // [ scriptable ]
  virtual ezVec3 GetTargetPosition() const = 0;                // [ scriptable ]
  virtual void ClearTargetPosition() = 0;                      // [ scriptable ]
  virtual ezAgentPathFindingState::Enum GetPathToTargetState() const = 0;

  ezEvent<const ezAgentSteeringEvent&> m_SteeringEvents;
};
