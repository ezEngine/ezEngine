#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

    ezSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<ezSceneExportModifier>();

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

void ezSceneExportModifier::ApplyAllModifiers(ezWorld& world, const ezUuid& documentGuid)
{
  ezHybridArray<ezSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(world, documentGuid);
  }

  DestroyModifiers(modifiers);
}
