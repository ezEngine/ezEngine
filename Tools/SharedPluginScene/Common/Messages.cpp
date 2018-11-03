#include <PCH.h>

#include <SharedPluginScene/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedSceneProperty, 1, ezRTTIDefaultAllocator<ezExposedSceneProperty>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Object", m_Object)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedDocumentObjectPropertiesMsgToEngine, 1, ezRTTIDefaultAllocator<ezExposedDocumentObjectPropertiesMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExportSceneGeometryMsgToEngine, 1, ezRTTIDefaultAllocator<ezExportSceneGeometryMsgToEngine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Selection", m_bSelectionOnly),
    EZ_MEMBER_PROPERTY("File", m_sOutputFile),
    EZ_MEMBER_PROPERTY("Mode", m_iExtractionMode),
    EZ_MEMBER_PROPERTY("Transform", m_Transform),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPullObjectStateMsgToEngine, 1, ezRTTIDefaultAllocator<ezPullObjectStateMsgToEngine>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPushObjectStateData, ezNoBase, 1, ezRTTIDefaultAllocator<ezPushObjectStateData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Guid", m_ObjectGuid),
    EZ_MEMBER_PROPERTY("Pos", m_vPosition),
    EZ_MEMBER_PROPERTY("Rot", m_qRotation)
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPushObjectStateMsgToEditor, 1, ezRTTIDefaultAllocator<ezPushObjectStateMsgToEditor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("States", m_ObjectStates)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on
