#include <PCH.h>
#include <SharedPluginAssets/Common/Messages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineRestoreResourceMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineRestoreResourceMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineResourceUpdateMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineResourceUpdateMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sResourceType),
    EZ_MEMBER_PROPERTY("Data", m_Data),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineRestartSimulationMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineRestartSimulationMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE