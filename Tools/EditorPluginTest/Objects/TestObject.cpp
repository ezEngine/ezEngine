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
  static int bla = 0;
  bla++;

  ezStringBuilder s;
  s.Format("Object %i", bla);

  SetName(s.GetData());
  m_bAwesome = true;
  m_fLoat = 23.0f;
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestObjectProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("FloatValue", m_fValue),
    EZ_MEMBER_PROPERTY("EditorProps", m_EditorProps),
    EZ_MEMBER_PROPERTY("Int8", m_Int8),
    EZ_MEMBER_PROPERTY("Int16", m_Int16),
    EZ_MEMBER_PROPERTY("UInt32", m_UInt32),
    EZ_MEMBER_PROPERTY("Blue", m_Color),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTestObjectProperties::ezTestObjectProperties()
{
  m_fValue = 42.0f;

  m_Int8 = 1;
  m_Int16 = 2;
  m_UInt32 = 3;
  m_Color = ezColor::CornflowerBlue; // The original!
}
