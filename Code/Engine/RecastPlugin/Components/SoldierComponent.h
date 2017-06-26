#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/Components/NpcComponent.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;
struct ezAgentSteeringEvent;

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezSoldierComponent, ezComponentUpdateType::WhenSimulating> ezSoldierComponentManager;

class EZ_RECASTPLUGIN_DLL ezSoldierComponent : public ezNpcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSoldierComponent, ezNpcComponent, ezSoldierComponentManager);

public:
  ezSoldierComponent();
  ~ezSoldierComponent();

  void Update();

  //////////////////////////////////////////////////////////////////////////
  // Properties




  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
protected:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // Steering

  void SteeringEventHandler(const ezAgentSteeringEvent& e);

  ezComponentHandle m_hSteeringComponent;

  //////////////////////////////////////////////////////////////////////////
  // Game State

  enum class State
  {
    Idle,
    WaitingForPath,
    Walking,
    ErrorState,
  };

  State m_State = State::Idle;



};
