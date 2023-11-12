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
  ezTransform m_Transform = ezTransform::MakeIdentity();
};

class EZ_JOLTPLUGIN_DLL ezJoltShapeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltShapeComponent, ezComponent);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltShapeComponent

public:
  ezJoltShapeComponent();
  ~ezJoltShapeComponent();

  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const {}

protected:
  friend class ezJoltActorComponent;
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) = 0;

  const ezJoltUserData* GetUserData();
  ezUInt32 GetUserDataIndex();

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
