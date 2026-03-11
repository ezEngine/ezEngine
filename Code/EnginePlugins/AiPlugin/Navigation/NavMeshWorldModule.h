#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Implementation/NavMeshGeneration.h>
#include <Core/World/WorldModule.h>

class ezAiNavMesh;
class dtNavMesh;

/// This world module keeps track of all the configured navmeshes (for different character types)
/// and makes sure to build their sectors in the background.
///
/// Through this you can get access to one of the available navmeshes.
/// Additionally, it also provides access to the different path search filters.
class EZ_AIPLUGIN_DLL ezAiNavMeshWorldModule final : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezAiNavMeshWorldModule, ezWorldModule);

public:
  ezAiNavMeshWorldModule(ezWorld* pWorld);
  ~ezAiNavMeshWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  ezAiNavMesh* GetNavMesh(ezStringView sName);
  const ezAiNavMesh* GetNavMesh(ezStringView sName) const;

  const dtQueryFilter& GetPathSearchFilter(ezStringView sName) const;

  const ezAiNavigationConfig& GetConfig() const { return m_Config; }

private:
  void Update(const UpdateContext& ctxt);

  ezMap<ezString, ezAiNavMesh*> m_WorldNavMeshes;

  // TODO: this is a hacky solution to delay the navmesh generation until after Physics has been set up.
  ezUInt32 m_uiUpdateDelay = 10;
  ezTaskGroupID m_GenerateSectorTaskID;
  ezSharedPtr<ezNavMeshSectorGenerationTask> m_pGenerateSectorTask;

  ezAiNavigationConfig m_Config;

  ezMap<ezString, dtQueryFilter> m_PathSearchFilters;
};

/* TODO:

Navmesh Generation
==================

* fix navmesh on hills
* collision group filtering
* invalidate sectors, re-generate
* Invalidate path searches after sector changes
* sector usage tracking
* unload unused sectors

Path Search
===========

* callback for touched sectors
* on-demand sector generation ???
* use max edge-length + poly flags for 'dynamic' obstacles

Steering
========

* movement with ineratia
* decoupled position and rotation
* avoid dynamic obstacles

*/
