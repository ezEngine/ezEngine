#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/HierarchyObject.h>

namespace ezModelImporter
{
  void ObjectHandle::operator = (const ObjectHandle& rhs)
  {
    m_Type = rhs.m_Type;
    m_Id = rhs.m_Id;
  }

  bool ObjectHandle::operator == (const ObjectHandle& rhs) const
  {
    return m_Type == rhs.m_Type && m_Id == rhs.m_Id;
  }

  void HierarchyObject::SetParent(ObjectHandle newParent)
  {
    EZ_ASSERT_DEBUG(newParent.GetType() == ObjectHandle::NODE, "Only nodes are valid parents for nodes.");
    m_Parent = newParent;
  }
}
