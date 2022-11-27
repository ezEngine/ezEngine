#pragma once

#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeConvexHullComponentManager = ezComponentManager<class ezJoltShapeConvexHullComponent, ezBlockStorageType::FreeList>;

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

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  ezJoltMeshResourceHandle GetMesh() const { return m_hCollisionMesh; }

protected:
  ezJoltMeshResourceHandle m_hCollisionMesh;
};
