#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class ezRecastWorldModule;
class ezAbstractObjectNode;

typedef ezTypedResourceHandle<class ezRecastNavMeshResource> ezRecastNavMeshResourceHandle;

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

  //////////////////////////////////////////////////////////////////////////
  // Editor Interface
private:

  void OnObjectCreated(const ezAbstractObjectNode& node);

protected:
  virtual void OnActivated() override;

  void VisualizeNavMesh();
  void VisualizePointsOfInterest();

  ezRecastNavMeshResourceHandle m_hNavMesh; 
};
