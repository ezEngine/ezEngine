#pragma once

#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeConvexHullComponentManager = ezComponentManager<class ezJoltShapeConvexHullComponent, ezBlockStorageType::FreeList>;

/// \brief Adds a Jolt convex hull shape to a Jolt actor.
///
/// A convex hull is a simple convex shape. It can be used for simulating dynamic rigid bodies.
/// Often the convex hull of a complex mesh is used to approximate the mesh and make it possible to use it as a dynamic actor.
class EZ_JOLTPLUGIN_DLL ezJoltShapeConvexHullComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltShapeConvexHullComponent, ezJoltShapeComponent, ezJoltShapeConvexHullComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltShapeComponent

protected:
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // ezConvexShapeConvexComponent

public:
  ezJoltShapeConvexHullComponent();
  ~ezJoltShapeConvexHullComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const override;

  ezJoltMeshResourceHandle GetMesh() const { return m_hCollisionMesh; }

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  ezJoltMeshResourceHandle m_hCollisionMesh;
};
