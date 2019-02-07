#include <EnginePluginScenePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezShapeIconComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute() // don't show in UI
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezShapeIconComponent::ezShapeIconComponent() {}

ezShapeIconComponent::~ezShapeIconComponent() {}

void ezShapeIconComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();
}

void ezShapeIconComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // ezStreamReader& s = stream.GetStream();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_RemoveShapeIconComponents, 1,
                                ezRTTIDefaultAllocator<ezSceneExportModifier_RemoveShapeIconComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


void ezSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(ezWorld& world, const ezUuid& documentGuid)
{
  EZ_LOCK(world.GetWriteMarker());

  ezShapeIconComponentManager* pSiMan = world.GetComponentManager<ezShapeIconComponentManager>();

  if (pSiMan == nullptr)
    return;

  ezUInt32 num = 0;

  for (auto it = pSiMan->GetComponents(); it.IsValid();)
  {
    ezShapeIconComponent* pComp = &(*it);
    it.Next();

    pSiMan->DeleteComponent(pComp->GetHandle());

    ++num;
  }

  // ezLog::Debug("Removed {0} shape icon components", num);
}
