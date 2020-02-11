#include <EditorPluginTypeScriptPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameter, 1, ezRTTINoAllocator);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameterNumber, 1, ezRTTIDefaultAllocator<ezTypeScriptParameterNumber>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameterBool, 1, ezRTTIDefaultAllocator<ezTypeScriptParameterBool>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameterString, 1, ezRTTIDefaultAllocator<ezTypeScriptParameterString>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameterVec3, 1, ezRTTIDefaultAllocator<ezTypeScriptParameterVec3>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptParameterColor, 1, ezRTTIDefaultAllocator<ezTypeScriptParameterColor>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetProperties, 1, ezRTTIDefaultAllocator<ezTypeScriptAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ScriptFile", m_sScriptFile)->AddAttributes(new ezFileBrowserAttribute("Select Script", "*.ts")),
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

ezTypeScriptAssetProperties::ezTypeScriptAssetProperties() = default;
ezTypeScriptAssetProperties::~ezTypeScriptAssetProperties() = default;
