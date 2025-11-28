#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshWriter.h>

EZ_IMPLEMENT_SINGLETON(ezJoltCollisionMeshWriter);

static ezJoltCollisionMeshWriter g_JoltCollisionMeshWriterSingleton;

ezJoltCollisionMeshWriter::ezJoltCollisionMeshWriter()
  : m_SingletonRegistrar(this)
{
}

ezJoltCollisionMeshWriter::~ezJoltCollisionMeshWriter() = default;
