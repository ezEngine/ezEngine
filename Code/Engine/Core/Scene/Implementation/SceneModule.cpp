#include <Core/PCH.h>
#include <Core/Scene/SceneModule.h>
#include <Core/World/World.h>
#include <Core/Scene/Scene.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneModule, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezWorld* ezSceneModule::GetWorld() const
{
  return m_pOwnerScene->GetWorld();
}
