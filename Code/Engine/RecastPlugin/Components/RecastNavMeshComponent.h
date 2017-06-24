#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>

class ezRecastWorldModule;

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezRcComponent, ezComponent);

public:
  ezRcComponent();
  ~ezRcComponent();

};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcNavMeshComponentManager : public ezComponentManager<class ezRcNavMeshComponent, ezBlockStorageType::Compact>
{
  typedef ezComponentManager<class ezRcNavMeshComponent, ezBlockStorageType::Compact> SUPER;

public:
  ezRcNavMeshComponentManager(ezWorld* pWorld);
  ~ezRcNavMeshComponentManager();

  virtual void Initialize() override;

  ezRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const ezWorldModule::UpdateContext& context);

private:
  ezRecastWorldModule* m_pWorldModule;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcNavMeshComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcNavMeshComponent, ezRcComponent, ezRcNavMeshComponentManager);

public:
  ezRcNavMeshComponent();
  ~ezRcNavMeshComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  bool m_bShowNavMesh = false;

  ezRecastConfig m_NavMeshConfig;

protected:
  virtual void OnSimulationStarted() override;
  void VisualizeNavMesh();
  void VisualizePointsOfInterest();

  ezUInt32 m_uiDelay = 2;
  ezRecastNavMeshBuilder m_NavMeshBuilder;
};
