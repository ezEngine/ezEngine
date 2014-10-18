#include <PCH.h>
#include <EditorPluginTest/Objects/TestObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestEditorProperties, ezDocumentObjectBaseProperties, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Awesome", m_bAwesome),
    EZ_MEMBER_PROPERTY("Pfloat", m_fLoat),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestEditorProperties::ezTestEditorProperties()
{
  SetName("TestEditorProperties");
  m_bAwesome = true;
  m_fLoat = 23.0f;
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestObjectProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("FloatValue", m_fValue),
    EZ_MEMBER_PROPERTY("EditorProps", m_EditorProps),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestObjectProperties::ezTestObjectProperties()
{
  m_fValue = 42.0f;
}

ezTestObject::ezTestObject() :
  m_ObjectProperties(&m_TestObjectProperties),
  m_EditorProperties(&m_TestEditorProperties)
{

}

