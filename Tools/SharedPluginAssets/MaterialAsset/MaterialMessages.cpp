#include <PCH.h>
#include <SharedPluginAssets/MaterialAsset/MaterialMessages.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineMaterialUpdateMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineMaterialUpdateMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Data", m_Data),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineRestoreResourceMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineRestoreResourceMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE

