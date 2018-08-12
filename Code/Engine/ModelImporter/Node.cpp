#include <PCH.h>

#include <ModelImporter/Node.h>
#include <ModelImporter/Scene.h>

namespace ezModelImporter
{
  Node::Node()
      : HierarchyObject(ObjectHandle::NODE)
  {
  }

  //  void Node::ComputeAbsoluteTransform(const Scene& scene, ezTransform& outAbsoluteTransform) const
  //  {
  //    ObjectHandle nextParent = m_Parent;
  //    if (nextParent.IsValid())
  //    {
  //      scene.GetObject<Node>(nextParent)->ComputeAbsoluteTransform(scene, outAbsoluteTransform);
  //    }
  //
  //    outAbsoluteTransform.SetGlobalTransform(outAbsoluteTransform, m_RelativeTransform);
  //  }
}
