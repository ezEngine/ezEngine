#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineRestartSimulationMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineRestartSimulationMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineLoopAnimationMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineLoopAnimationMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorEngineSetMaterialsMsg, 1, ezRTTIDefaultAllocator<ezEditorEngineSetMaterialsMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
