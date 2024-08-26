#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeSphereComponentManager = ezComponentManager<class ezJoltShapeSphereComponent, ezBlockStorageType::FreeList>;

/// \brief Adds a Jolt sphere shape to a Jolt actor.
class EZ_JOLTPLUGIN_DLL ezJoltShapeSphereComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltShapeSphereComponent, ezJoltShapeComponent, ezJoltShapeSphereComponentManager);

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
  // ezJoltShapeSphereComponent

public:
  ezJoltShapeSphereComponent();
  ~ezJoltShapeSphereComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
};
