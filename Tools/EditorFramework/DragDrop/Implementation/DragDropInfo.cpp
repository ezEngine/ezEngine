#include <PCH.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragDropInfo, 1, ezRTTIDefaultAllocator<ezDragDropInfo>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDragDropInfo::ezDragDropInfo()
{
  m_vDropPosition.Set(ezMath::BasicType<float>::GetNaN());
  m_vDropNormal.Set(ezMath::BasicType<float>::GetNaN());
  m_iTargetObjectSubID = -1;
  m_iTargetObjectInsertChildIndex = -1;
}


