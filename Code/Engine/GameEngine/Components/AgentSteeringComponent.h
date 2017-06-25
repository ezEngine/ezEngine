#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

/// \brief Base class for components that implement 'agent steering' behavior.
/// If, moving from point A to point B using something like a navmesh.
class EZ_GAMEENGINE_DLL ezAgentSteeringComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezAgentSteeringComponent, ezComponent);

public:
  ezAgentSteeringComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
};


