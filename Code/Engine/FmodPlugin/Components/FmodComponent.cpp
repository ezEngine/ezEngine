#include <PCH.h>
#include <FmodPlugin/Components/FmodComponent.h>
#include <FmodPlugin/FmodIncludes.h>

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezFmodComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sound"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE





EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodComponent);

