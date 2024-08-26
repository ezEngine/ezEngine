#pragma once

#include <JoltPlugin/Shapes/JoltShapeComponent.h>

using ezJoltShapeBoxComponentManager = ezComponentManager<class ezJoltShapeBoxComponent, ezBlockStorageType::FreeList>;

/// \brief Adds a Jolt box shape to a Jolt actor.
class EZ_JOLTPLUGIN_DLL ezJoltShapeBoxComponent : public ezJoltShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltShapeBoxComponent, ezJoltShapeComponent, ezJoltShapeBoxComponentManager);

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
  // ezJoltShapeBoxComponent

public:
  ezJoltShapeBoxComponent();
  ~ezJoltShapeBoxComponent();

  void SetHalfExtents(const ezVec3& value);                       // [ property ]
  const ezVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const override;

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  ezVec3 m_vHalfExtents = ezVec3(0.5f);
};
