#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/SceneExport/JoltFinalizeGeneratedCollision.h>
#include <JoltPlugin/Components/JoltGenerateCollisionComponent.h>
#include <RendererCore/Meshes/SplineMeshComponent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_JoltFinalizeGeneratedCollision, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_JoltFinalizeGeneratedCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezSceneExportModifier_JoltFinalizeGeneratedCollision::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  // Don't finalize yet for prefabs since the real generation might only happen in the final scene context
  if (sDocumentType == "Prefab" && bForExport)
  {
    return;
  }

  EZ_LOCK(ref_world.GetWriteMarker());

  // Spline mesh generation runs asynchronously and requests the collision generation via a posted message,
  // make sure all of that has happened before the collision is finalized.
  if (bForExport)
  {
    if (auto pSplineMeshManager = ref_world.GetComponentManager<ezSplineMeshComponentManager>())
    {
      for (auto it = pSplineMeshManager->GetComponents(); it.IsValid(); ++it)
      {
        it->EnsureGenerationFinished();
      }
    }

    ref_world.Update();
  }

  auto pComponentManager = ref_world.GetComponentManager<ezJoltGenerateCollisionComponentManager>();
  if (pComponentManager == nullptr)
    return;

  for (auto it = pComponentManager->GetComponents(); it.IsValid(); ++it)
  {
    it->FinalizeGeneration();

    pComponentManager->DeleteComponent(it);
  }
}
