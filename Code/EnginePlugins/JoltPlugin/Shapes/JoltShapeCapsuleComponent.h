#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeCapsuleComponentManager = ezComponentManager<class ezJoltShapeCapsuleComponent, ezBlockStorageType::FreeList>;

class EZ_JOLTPLUGIN_DLL ezJoltShapeCapsuleComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltShapeCapsuleComponent, ezJoltShapeComponent, ezJoltShapeCapsuleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltShapeComponent

protected:
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltShapeCapsuleComponent

public:
  ezJoltShapeCapsuleComponent();
  ~ezJoltShapeCapsuleComponent();

  void SetRadius(float f);                      // [ property ]
  float GetRadius() const { return m_fRadius; } // [ property ]

  void SetHeight(float f);                      // [ property ]
  float GetHeight() const { return m_fHeight; } // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  float m_fRadius = 0.5f;
  float m_fHeight = 0.5f;
};
