#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragDropInfo, 1, ezRTTIDefaultAllocator<ezDragDropInfo>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDragDropInfo::ezDragDropInfo()
{
  m_vDropPosition.Set(ezMath::NaN<float>());
  m_vDropNormal.Set(ezMath::NaN<float>());
  m_iTargetObjectSubID = -1;
  m_iTargetObjectInsertChildIndex = -1;
  m_bShiftKeyDown = false;
  m_bCtrlKeyDown = false;
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDragDropConfig, 1, ezRTTIDefaultAllocator<ezDragDropConfig>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDragDropConfig::ezDragDropConfig()
{
  m_bPickSelectedObjects = false;
}

void operator>>(QDataStream& stream, ezDynamicArray<ezDocumentObject*>& rhs)
{
  int iIndices = 0;
  stream >> iIndices;
  rhs.Clear();
  rhs.Reserve(static_cast<ezUInt32>(iIndices));

  for (int i = 0; i < iIndices; ++i)
  {
    void* p = nullptr;

    uint len = sizeof(void*);
    stream.readRawData((char*)&p, len);

    ezDocumentObject* pDocObject = (ezDocumentObject*)p;

    rhs.PushBack(pDocObject);
  }
}
