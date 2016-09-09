#pragma once

#include <Foundation/Math/Transform.h>
#include <EditorPluginAssets/ModelImporter/HierarchyObject.h>

namespace ezModelImporter
{
  /// A node in the hierarchy of an imported scene.
  class Node : public HierarchyObject
  {
  public:
    Node();

    /// Relative transform of this node to its parent.
    ezTransform m_RelativeTransform;

    /// Children.
    ezDynamicArray<ObjectHandle> m_Children;
  };
}
