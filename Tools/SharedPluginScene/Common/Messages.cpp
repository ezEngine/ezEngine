#include <PCH.h>
#include <SharedPluginScene/Common/Messages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedSceneProperty, 1, ezRTTIDefaultAllocator<ezExposedSceneProperty>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Object", m_Object),
    EZ_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedDocumentObjectPropertiesMsgToEngine, 1, ezRTTIDefaultAllocator<ezExposedDocumentObjectPropertiesMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
