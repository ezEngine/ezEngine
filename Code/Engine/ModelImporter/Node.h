#pragma once

#include <ModelImporter/Plugin.h>
#include <ModelImporter/HierarchyObject.h>
#include <Foundation/Math/Transform.h>

namespace ezModelImporter
{
  /// A node in the hierarchy of an imported scene.
  class EZ_MODELIMPORTER_DLL Node : public HierarchyObject
  {
  public:
    Node();

    //void ComputeAbsoluteTransform(const Scene& scene, ezTransform& outAbsoluteTransform) const;

    /// Relative transform of this node to its parent.
    ezTransform m_RelativeTransform;

    /// Children. ImporterImplementations are responsible to avoid cycles.
    ezDynamicArray<ObjectHandle> m_Children;

    struct Metadata
    {
      ezString m_Key;
      ezVariant m_Data;
    };

    /// Meta data. May contain information that otherwise do not map into ezModelImporter's data structure.
    ezDynamicArray<Metadata> m_Metadata;
  };
}
