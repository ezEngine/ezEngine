#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameter, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameterNumber, 1, ezRTTIDefaultAllocator<ezDLangParameterNumber>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameterBool, 1, ezRTTIDefaultAllocator<ezDLangParameterBool>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameterString, 1, ezRTTIDefaultAllocator<ezDLangParameterString>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameterVec3, 1, ezRTTIDefaultAllocator<ezDLangParameterVec3>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangParameterColor, 1, ezRTTIDefaultAllocator<ezDLangParameterColor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangAssetProperties, 1, ezRTTIDefaultAllocator<ezDLangAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ScriptFile", m_sScriptFile)->AddAttributes(new ezFileBrowserAttribute("Select Script", "*.d")),
    EZ_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    EZ_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    EZ_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters),
    EZ_ARRAY_MEMBER_PROPERTY("Vec3Parameters", m_Vec3Parameters),
    EZ_ARRAY_MEMBER_PROPERTY("ColorParameters", m_ColorParameters),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDLangAssetProperties::ezDLangAssetProperties() = default;
ezDLangAssetProperties::~ezDLangAssetProperties() = default;
