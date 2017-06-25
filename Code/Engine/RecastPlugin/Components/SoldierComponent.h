#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/Components/NpcComponent.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezSoldierComponent, ezComponentUpdateType::WhenSimulating> ezSoldierComponentManager;

class EZ_RECASTPLUGIN_DLL ezSoldierComponent : public ezNpcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSoldierComponent, ezNpcComponent, ezSoldierComponentManager);

public:
  ezSoldierComponent();
  ~ezSoldierComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  //////////////////////////////////////////////////////////////////////////
  // Properties


protected:
  virtual void OnSimulationStarted() override;


};
