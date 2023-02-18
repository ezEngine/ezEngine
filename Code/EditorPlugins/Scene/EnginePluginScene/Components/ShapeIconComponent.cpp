#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezShapeIconComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Editing Utilities"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezShapeIconComponent::ezShapeIconComponent() = default;
ezShapeIconComponent::~ezShapeIconComponent() = default;

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_RemoveShapeIconComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_RemoveShapeIconComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(ezWorld& ref_world, const ezUuid& documentGuid, bool bForExport)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  if (ezShapeIconComponentManager* pSiMan = ref_world.GetComponentManager<ezShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}
