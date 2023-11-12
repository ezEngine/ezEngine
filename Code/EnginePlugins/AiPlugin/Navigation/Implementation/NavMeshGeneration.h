#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Threading/TaskSystem.h>

class ezPhysicsWorldModuleInterface;

class ezNavMeshSectorGenerationTask : public ezTask
{
public:
  ezAiNavMesh::SectorID m_SectorID = ezInvalidIndex;
  ezAiNavMesh* m_pWorldNavMesh = nullptr;
  const ezPhysicsWorldModuleInterface* m_pPhysics = nullptr;

protected:
  virtual void Execute() override;
};
