#include <PCH.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>

ezNavMeshDescription::ezNavMeshDescription() { }
ezNavMeshDescription::~ezNavMeshDescription() { }

void ezNavMeshDescription::Clear()
{
  m_BoxObstacles.Clear();
  m_Vertices.Clear();
  m_Triangles.Clear();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_AI_NavMesh_NavMeshDescription);

