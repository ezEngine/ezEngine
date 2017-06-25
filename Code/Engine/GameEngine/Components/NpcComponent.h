#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

/// \brief Base class for all components that implement 'non player character' behavior.
/// Ie, game logic for how an AI controlled character should behave (state machines etc)
class EZ_GAMEENGINE_DLL ezNpcComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezNpcComponent, ezComponent);

public:
  ezNpcComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
};


