#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Component.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct ezMsgExtractGeometry;
struct ezMsgUpdateLocalBounds;
class ezJoltUserData;
class ezJoltMaterial;

namespace JPH
{
  class Shape;
}

struct ezJoltSubShape
{
  JPH::Shape* m_pShape = nullptr;
  ezTransform m_Transform = ezTransform::IdentityTransform();
};

class EZ_JOLTPLUGIN_DLL ezJoltShapeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltShapeComponent, ezComponent);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltShapeComponent

public:
  ezJoltShapeComponent();
  ~ezJoltShapeComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& msg) const {}

  //ezBitflags<ezOnJoltContact> m_OnContact; // [ property ]

protected:
  friend class ezJoltActorComponent;
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) = 0;

  const ezJoltUserData* GetUserData();
  ezUInt32 GetUserDataIndex();

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
