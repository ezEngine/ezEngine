#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class ezRecastWorldModule;
class ezAbstractObjectNode;

using ezRecastNavMeshResourceHandle = ezTypedResourceHandle<class ezRecastNavMeshResource>;

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for all Recast components
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
  using SUPER = ezComponentManager<class ezRcNavMeshComponent, ezBlockStorageType::Compact>;

public:
  ezRcNavMeshComponentManager(ezWorld* pWorld);
  ~ezRcNavMeshComponentManager();

  virtual void Initialize() override;

  ezRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const ezWorldModule::UpdateContext& context);

private:
  ezRecastWorldModule* m_pWorldModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcNavMeshComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcNavMeshComponent, ezRcComponent, ezRcNavMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  //  ezRcNavMeshComponent

public:
  ezRcNavMeshComponent();
  ~ezRcNavMeshComponent();

  bool m_bShowNavMesh = false;    // [ property ]

  ezRecastConfig m_NavMeshConfig; // [ property ]

protected:
  void Update();
  void VisualizeNavMesh();
  void VisualizePointsOfInterest();

  ezRecastNavMeshResourceHandle m_hNavMesh;


  //////////////////////////////////////////////////////////////////////////
  // Editor

protected:
  void OnObjectCreated(const ezAbstractObjectNode& node);
};
