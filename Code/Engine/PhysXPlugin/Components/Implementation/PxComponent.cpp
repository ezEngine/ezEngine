#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxComponent.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxComponent, 1)
{
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_ABSTRACT_COMPONENT_TYPE





EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxComponent);

