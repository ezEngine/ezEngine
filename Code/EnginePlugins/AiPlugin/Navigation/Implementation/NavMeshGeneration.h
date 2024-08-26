#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Threading/TaskSystem.h>

class ezNavmeshGeoWorldModuleInterface;

class ezNavMeshSectorGenerationTask : public ezTask
{
public:
  ezAiNavMesh::SectorID m_SectorID = ezInvalidIndex;
  ezAiNavMesh* m_pWorldNavMesh = nullptr;
  const ezNavmeshGeoWorldModuleInterface* m_pNavGeo = nullptr;

protected:
  virtual void Execute() override;
};
