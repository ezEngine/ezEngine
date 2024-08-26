#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/Bitflags.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

namespace JPH
{
  class Shape;
  class BodyCreationSettings;
} // namespace JPH

/// \brief Base class for all Jolt actors.
///
/// An actor is an object that participates in the physical simulation.
/// It is often also called a (rigid) body.
/// An actor is made out of one or multiple shapes that define its geometry.
/// Different types of actors differ in how they participate in the simulation.
class EZ_JOLTPLUGIN_DLL ezJoltActorComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltActorComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltActorComponent

public:
  ezJoltActorComponent();
  ~ezJoltActorComponent();

  /// \brief The collision layer determines with which other actors this actor collides.
  ///
  /// Which collision layers collide with each other is configured through the ezCollisionFilterConfig.
  /// \see ezJoltCollisionFiltering::GetCollisionFilterConfig()
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// \brief Sets the object filter ID to use. This can only be set right after creation, before the component gets activated.
  void SetInitialObjectFilterID(ezUInt32 uiObjectFilterID);

  /// \brief The object filter ID can be used to ignore collisions specifically with this one object.
  ezUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

  /// \brief Returns the internal ID used by Jolt to identify this actor/body.
  ezUInt32 GetJoltBodyID() const { return m_uiJoltBodyID; }

protected:
  const ezJoltUserData* GetUserData() const;

  void ExtractSubShapeGeometry(const ezGameObject* pObject, ezMsgExtractGeometry& msg) const;

  static void GatherShapes(ezDynamicArray<ezJoltSubShape>& shapes, ezGameObject* pObject, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial);
  ezResult CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const ezJoltMaterial* pMaterial);

  virtual void CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial) {}

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;
};
