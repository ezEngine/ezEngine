#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/WorldModule.h>
#include <NavMeshBuilder/NavMeshPointsOfInterest.h>

class dtCrowd;
class dtNavMesh;

class EZ_RECASTPLUGIN_DLL ezRecastWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezRecastWorldModule, ezWorldModule);

public:
  ezRecastWorldModule(ezWorld* pWorld);
  ~ezRecastWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void SetNavMesh(dtNavMesh* pNavMesh);
  dtNavMesh* GetNavMesh() const { return m_pNavMesh; }

  bool IsInitialized() const { return m_pCrowd != nullptr; }

  dtCrowd* m_pCrowd = nullptr;

  ezNavMeshPointOfInterestGraph m_NavMeshPointsOfInterest;

private:
  void UpdateCrowd(const UpdateContext& ctxt);

  dtNavMesh* m_pNavMesh = nullptr;
};

