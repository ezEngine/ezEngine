#include <PCH.h>
#include <EditorFramework/SceneExport/SceneExportModifier.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE


void ezSceneExportModifier::CreateModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers)
{
  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezSceneExportModifier>())
      continue;

    if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    if (pRtti->GetAllocator() == nullptr || !pRtti->GetAllocator()->CanAllocate())
      continue;

    ezSceneExportModifier* pMod = static_cast<ezSceneExportModifier*>(pRtti->GetAllocator()->Allocate());

    modifiers.PushBack(pMod);
  }
}

void ezSceneExportModifier::DestroyModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers)
{
  for (auto pMod : modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  modifiers.Clear();
}

void ezSceneExportModifier::ApplyAllModifiers(ezWorld& world)
{
  ezHybridArray<ezSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(world);
  }

  DestroyModifiers(modifiers);
}
