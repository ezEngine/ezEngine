#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezRcMarkPoiVisibleComponent, ezComponentUpdateType::WhenSimulating> ezRcMarkPoiVisibleComponentManager;

class EZ_RECASTPLUGIN_DLL ezRcMarkPoiVisibleComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcMarkPoiVisibleComponent, ezRcComponent, ezRcMarkPoiVisibleComponentManager);

public:
  ezRcMarkPoiVisibleComponent();
  ~ezRcMarkPoiVisibleComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  float m_fRadius = 20.0f;
  ezUInt8 m_uiCollisionLayer = 0;

protected:
  ezRecastWorldModule* m_pWorldModule = nullptr;
  ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

  virtual void OnSimulationStarted() override;

};
