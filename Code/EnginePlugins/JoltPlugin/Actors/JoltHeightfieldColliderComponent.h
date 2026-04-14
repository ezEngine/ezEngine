#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

using ezJoltHeightfieldColliderComponentManager = ezComponentManager<class ezJoltHeightfieldColliderComponent, ezBlockStorageType::FreeList>;

/// Manages a Jolt static body with a heightfield shape, attached programmatically via ezPhysicsWorldModuleInterface.
///
/// This component is not added through the editor. Use ezPhysicsWorldModuleInterface::CreateHeightfieldCollider()
/// or TrySetHeightfieldCollider() to create and attach it to a game object. It owns the associated Jolt body
/// and removes it from the physics system when deactivated or deleted.
class EZ_JOLTPLUGIN_DLL ezJoltHeightfieldColliderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltHeightfieldColliderComponent, ezComponent, ezJoltHeightfieldColliderComponentManager);

protected:
  virtual void OnDeactivated() override;

public:
  ezJoltHeightfieldColliderComponent();
  ~ezJoltHeightfieldColliderComponent();

private:
  friend class ezJoltWorldModule;

  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiObjectFilterID = ezInvalidIndex;

  /// Identifier of the shape cache entry this body was created from. Empty if the shape is not cached.
  ezString m_sCacheIdentifier;
};
