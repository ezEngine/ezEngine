#include <PCH.h>
#include <EditorFramework/GUI/ExposedParameters.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezExposedParameter, ezNoBase, 1, ezRTTIDefaultAllocator<ezExposedParameter>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedParameters, 2, ezRTTIDefaultAllocator<ezExposedParameters>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezExposedParameters::ezExposedParameters()
{

}
