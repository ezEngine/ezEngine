#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

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

  CleanUpWorld(world);
}

void VisitObject(ezWorld& world, ezGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!ezStringUtils::IsNullOrEmpty(pObject->GetName()))
    return;

  if (!ezStringUtils::IsNullOrEmpty(pObject->GetGlobalKey()))
    return;

  world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void ezSceneExportModifier::CleanUpWorld(ezWorld& world)
{
  EZ_LOCK(world.GetWriteMarker());

  for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  {
    // only visit objects without parents, those are the root objects
    if (it->GetParent() != nullptr)
      continue;

    VisitObject(world, it);
  }

  world.SetWorldSimulationEnabled(false);
  world.Update();
}
