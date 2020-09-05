#pragma once

#include <Foundation/Math/Transform.h>
#include <ModelImporter/HierarchyObject.h>
#include <ModelImporter/ModelImporterDLL.h>

namespace ezModelImporter
{
  /// A node in the hierarchy of an imported scene.
  class EZ_MODELIMPORTER_DLL Node : public HierarchyObject
  {
  public:
    Node();

    // void ComputeAbsoluteTransform(const Scene& scene, ezTransform& outAbsoluteTransform) const;

    /// Relative transform of this node to its parent.
    ezTransform m_RelativeTransform;

    /// Children. ImporterImplementations are responsible to avoid cycles.
    ezDynamicArray<ObjectHandle> m_Children;
  };
} // namespace ezModelImporter
