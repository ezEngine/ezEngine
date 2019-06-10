#pragma once

#include <RecastPlugin/RecastPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <NavMeshBuilder/NavMeshPointsOfInterest.h>

class dtCrowd;
class dtNavMesh;
struct ezResourceEvent;

typedef ezTypedResourceHandle<class ezRecastNavMeshResource> ezRecastNavMeshResourceHandle;

class EZ_RECASTPLUGIN_DLL ezRecastWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastWorldModule, ezWorldModule);

public:
  ezRecastWorldModule(ezWorld* pWorld);
  ~ezRecastWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void SetNavMeshResource(const ezRecastNavMeshResourceHandle& hNavMesh);
  const ezRecastNavMeshResourceHandle& GetNavMeshResource() { return m_hNavMesh; }

  const dtNavMesh* GetDetourNavMesh() const { return m_pDetourNavMesh; }
  const ezNavMeshPointOfInterestGraph* GetNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }
  ezNavMeshPointOfInterestGraph* AccessNavMeshPointsOfInterestGraph() const { return m_pNavMeshPointsOfInterest.Borrow(); }

private:
  void UpdateNavMesh(const UpdateContext& ctxt);
  void ResourceEventHandler(const ezResourceEvent& e);

  const dtNavMesh* m_pDetourNavMesh = nullptr;
  ezRecastNavMeshResourceHandle m_hNavMesh;
  ezUniquePtr<ezNavMeshPointOfInterestGraph> m_pNavMeshPointsOfInterest;
};
