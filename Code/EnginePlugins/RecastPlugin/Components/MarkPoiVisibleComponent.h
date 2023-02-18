#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

typedef ezComponentManagerSimple<class ezRcMarkPoiVisibleComponent, ezComponentUpdateType::WhenSimulating> ezRcMarkPoiVisibleComponentManager;

class EZ_RECASTPLUGIN_DLL ezRcMarkPoiVisibleComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcMarkPoiVisibleComponent, ezRcComponent, ezRcMarkPoiVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRcMarkPoiVisibleComponent

public:
  ezRcMarkPoiVisibleComponent();
  ~ezRcMarkPoiVisibleComponent();

  float m_fRadius = 20.0f;        // [ property ]
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

protected:
  void Update();

  ezRecastWorldModule* m_pWorldModule = nullptr;
  ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

private:
  ezUInt32 m_uiLastFirstCheckedPoint = 0;
};
