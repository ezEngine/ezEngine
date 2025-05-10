#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetProcessorMessages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessAssetMsg, 1, ezRTTIDefaultAllocator<ezProcessAssetMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
    EZ_MEMBER_PROPERTY("AssetHash", m_AssetHash),
    EZ_MEMBER_PROPERTY("ThumbHash", m_ThumbHash),
    EZ_MEMBER_PROPERTY("PackageHash", m_PackageHash),
    EZ_MEMBER_PROPERTY("AssetPath", m_sAssetPath),
    EZ_MEMBER_PROPERTY("Platform", m_sPlatform),
    EZ_ARRAY_MEMBER_PROPERTY("DepRefHull", m_DepRefHull),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessAssetResponseMsg, 1, ezRTTIDefaultAllocator<ezProcessAssetResponseMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Status", m_Status),
    EZ_ARRAY_MEMBER_PROPERTY("LogEntries", m_LogEntries),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFreeAllResourcesMsg, 1, ezRTTIDefaultAllocator<ezFreeAllResourcesMsg>)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
