#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier::CreateModifiers(ezHybridArray<ezSceneExportModifier*, 8>& ref_modifiers)
{
  ezRTTI::ForEachDerivedType<ezSceneExportModifier>(
    [&](const ezRTTI* pRtti)
    {
      ezSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<ezSceneExportModifier>();
      ref_modifiers.PushBack(pMod);
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void ezSceneExportModifier::DestroyModifiers(ezHybridArray<ezSceneExportModifier*, 8>& ref_modifiers)
{
  for (auto pMod : ref_modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  ref_modifiers.Clear();
}

void ezSceneExportModifier::ApplyAllModifiers(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  ezHybridArray<ezSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(ref_world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(ref_world);
}

void VisitObject(ezWorld& ref_world, ezGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(ref_world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!pObject->GetName().IsEmpty())
    return;

  if (!pObject->GetGlobalKey().IsEmpty())
    return;

  ref_world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void ezSceneExportModifier::CleanUpWorld(ezWorld& ref_world)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  // Don't do this (for now), as we would also delete objects that are referenced by other components,
  // and currently we can't know which ones are important to keep.

  // for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  //{
  //   // only visit objects without parents, those are the root objects
  //   if (it->GetParent() != nullptr)
  //     continue;

  //  VisitObject(world, it);
  //}

  const bool bSim = ref_world.GetWorldSimulationEnabled();
  ref_world.SetWorldSimulationEnabled(false);
  ref_world.Update();
  ref_world.SetWorldSimulationEnabled(bSim);
}
