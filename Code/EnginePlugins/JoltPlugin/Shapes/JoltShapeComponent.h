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

/// \brief Base class for all Jolt physics shapes.
///
/// A physics shape is used to represent (part of) a physical actor, such as a box or sphere,
/// which is used for the rigid body simulation.
///
/// When an actor is created, it searches for ezJoltShapeComponent on its own object and all child objects.
/// It then adds all these shapes, with their respective transforms, to the Jolt actor.
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

  /// \brief If overridden, a triangular representation of the physics shape is added to the geometry object.
  ///
  /// This may be used for debug visualization or navmesh generation (though there are also other ways to do those).
  virtual void ExtractGeometry(ezMsgExtractGeometry& ref_msg) const {}

protected:
  friend class ezJoltActorComponent;
  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) = 0;

  const ezJoltUserData* GetUserData();
  ezUInt32 GetUserDataIndex();

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
