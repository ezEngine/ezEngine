#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/CustomData/SampleCustomData.h>

// BEGIN-DOCS-CODE-SNIPPET: customdata-impl
// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleCustomData, 1, ezRTTIDefaultAllocator<SampleCustomData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Text", m_sText),
    EZ_MEMBER_PROPERTY("Size", m_iSize)->AddAttributes(new ezDefaultValueAttribute(42), new ezClampValueAttribute(16, 64)),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::CornflowerBlue)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_DEFINE_CUSTOM_DATA_RESOURCE(SampleCustomData);
// END-DOCS-CODE-SNIPPET
