#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTransformResult, 1)
  EZ_ENUM_CONSTANT(ezTransformResult::Success),
  EZ_ENUM_CONSTANT(ezTransformResult::Failure),
  EZ_ENUM_CONSTANT(ezTransformResult::NeedsImport),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTransformStatus, ezNoBase, 1, ezRTTIDefaultAllocator<ezTransformStatus>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Result", ezTransformResult, m_Result),
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on
