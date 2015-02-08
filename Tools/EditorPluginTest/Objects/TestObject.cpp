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

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezExampleEnum, 1)
  EZ_ENUM_CONSTANTS(ezExampleEnum::Value1, ezExampleEnum::Value2) 
  EZ_ENUM_CONSTANT(ezExampleEnum::Value3),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezExampleBitflags, 1)
  EZ_BITFLAGS_CONSTANTS(ezExampleBitflags::Value1, ezExampleBitflags::Value2) 
  EZ_BITFLAGS_CONSTANT(ezExampleBitflags::Value3),
EZ_END_STATIC_REFLECTED_BITFLAGS();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTestObjectProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("FloatValue", m_fValue),
    EZ_MEMBER_PROPERTY("EditorProps", m_EditorProps),
    EZ_MEMBER_PROPERTY("Int8", m_Int8),
    EZ_MEMBER_PROPERTY("Int16", m_Int16),
    EZ_MEMBER_PROPERTY("UInt32", m_UInt32),
    EZ_MEMBER_PROPERTY("Blue", m_Color),
    EZ_ENUM_MEMBER_PROPERTY("Enum", ezExampleEnum, m_Enum),
    EZ_BITFLAGS_MEMBER_PROPERTY("Bitflags", ezExampleBitflags, m_Bitflags),
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
