#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>

class ezRecastWorldModule;

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponentManager : public ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::Compact>
{
  typedef ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::Compact> SUPER;

public:
  ezRcAgentComponentManager(ezWorld* pWorld);
  ~ezRcAgentComponentManager();

  virtual void Initialize() override;

  ezRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const ezWorldModule::UpdateContext& context);

private:
  ezRecastWorldModule* m_pWorldModule;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcAgentComponent, ezRcComponent, ezRcAgentComponentManager);

public:
  ezRcAgentComponent();
  ~ezRcAgentComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

protected:
  virtual void OnSimulationStarted() override;

  ezInt32 m_iAgentIndex = -1;
  ezTime m_tLastUpdate;
};
