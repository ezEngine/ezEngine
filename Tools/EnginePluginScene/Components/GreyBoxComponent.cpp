#include <PCH.h>
#include <EnginePluginScene/Components/GreyBoxComponent.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGreyBoxShape, 1)
EZ_ENUM_CONSTANTS(ezGreyBoxShape::Box)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_COMPONENT_TYPE(ezGreyBoxComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Shape", ezGreyBoxShape, m_Shape),
    EZ_MEMBER_PROPERTY("SizePosX", m_fSizePosX),
    EZ_MEMBER_PROPERTY("SizeNegX", m_fSizeNegX),
    EZ_MEMBER_PROPERTY("SizePosY", m_fSizePosY),
    EZ_MEMBER_PROPERTY("SizeNegY", m_fSizeNegY),
    EZ_MEMBER_PROPERTY("SizePosZ", m_fSizePosZ),
    EZ_MEMBER_PROPERTY("SizeNegZ", m_fSizeNegZ),
  }
    EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute() // don't show in UI
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezGreyBoxComponent::ezGreyBoxComponent()
{
}

ezGreyBoxComponent::~ezGreyBoxComponent()
{
}

void ezGreyBoxComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  //ezStreamWriter& s = stream.GetStream();
}

void ezGreyBoxComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  //ezStreamReader& s = stream.GetStream();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_ConvertGreyBoxComponents, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_ConvertGreyBoxComponents>)
EZ_END_DYNAMIC_REFLECTED_TYPE


void ezSceneExportModifier_ConvertGreyBoxComponents::ModifyWorld(ezWorld& world)
{
  EZ_LOCK(world.GetWriteMarker());

  ezGreyBoxComponentManager* pMan = world.GetComponentManager<ezGreyBoxComponentManager>();

  if (pMan == nullptr)
    return;

  ezUInt32 num = 0;

  for (auto it = pMan->GetComponents(); it.IsValid(); )
  {
    ezGreyBoxComponent* pComp = &(*it);
    it.Next();

    pMan->DeleteComponent(pComp->GetHandle());

    ++num;
  }
}
