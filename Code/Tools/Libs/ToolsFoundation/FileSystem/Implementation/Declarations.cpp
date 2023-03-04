#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFileStatus, ezNoBase, 3, ezRTTIDefaultAllocator<ezFileStatus>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Timestamp", m_Timestamp),
      EZ_MEMBER_PROPERTY("Hash", m_uiHash),
      EZ_MEMBER_PROPERTY("DocumentID", m_DocumentID),
    } EZ_END_PROPERTIES;
  }
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

