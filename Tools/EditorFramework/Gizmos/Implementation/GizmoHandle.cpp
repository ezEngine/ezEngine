#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorGizmoHandle, ezEditorEngineSyncObject, 1, ezRTTIDefaultAllocator<ezEditorGizmoHandle>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Visible", m_bVisible),
    EZ_MEMBER_PROPERTY("Transformation", m_Transformation),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();
