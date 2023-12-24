#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeCylinderComponentManager = ezComponentManager<class ezJoltShapeCylinderComponent, ezBlockStorageType::FreeList>;

/// \brief Adds a Jolt cylinder shape to a Jolt actor.
///
/// Be aware that the cylinder shape is not as stable in simulation as other shapes.
/// If possible use capsule shapes instead. In some cases even using a convex hull shape may provide better results,
/// but this has to be tried out case by case.
class EZ_JOLTPLUGIN_DLL ezJoltShapeCylinderComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltShapeCylinderComponent, ezJoltShapeComponent, ezJoltShapeCylinderComponentManager);

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
  // ezJoltShapeCylinderComponent

public:
  ezJoltShapeCylinderComponent();
  ~ezJoltShapeCylinderComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
