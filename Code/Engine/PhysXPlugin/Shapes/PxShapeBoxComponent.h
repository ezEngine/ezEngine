#pragma once

#include <PhysXPlugin/Shapes/PxShapeComponent.h>

typedef ezComponentManager<class ezPxShapeBoxComponent> ezPxShapeBoxComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxShapeBoxComponent : public ezPxShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxShapeBoxComponent, ezPxShapeComponent, ezPxShapeBoxComponentManager);

public:
  ezPxShapeBoxComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;

  // ************************************* PROPERTIES ***********************************
public:

  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const { return m_vExtents; }

protected:
  ezVec3 m_vExtents;


  // ************************************* FUNCTIONS *****************************

public:

  virtual void AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform) override;

protected:

};


