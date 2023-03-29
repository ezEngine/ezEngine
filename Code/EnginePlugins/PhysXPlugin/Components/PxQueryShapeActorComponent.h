#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

class ezPxQueryShapeActorComponent;

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxQueryShapeActorComponentManager : public ezComponentManager<ezPxQueryShapeActorComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxQueryShapeActorComponentManager(ezWorld* pWorld);
  ~ezPxQueryShapeActorComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxQueryShapeActorComponent;

  void UpdateKinematicActors();

  ezDynamicArray<ezPxQueryShapeActorComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A physics actor that can be moved procedurally (like a kinematic actor) but that doesn't affect rigid bodies.
///
/// It passes right through dynamic actors. However, you can detect it via raycasts or shape casts.
/// This is useful to represent detail shapes (like the collision shapes of animated meshes) that should be pickable,
/// but that shouldn't interact with the world otherwise.
/// They are more lightweight at runtime than full kinematic dynamic actors.
class EZ_PHYSXPLUGIN_DLL ezPxQueryShapeActorComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxQueryShapeActorComponent, ezPxActorComponent, ezPxQueryShapeActorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxQueryShapeActorComponent

public:
  ezPxQueryShapeActorComponent();
  ~ezPxQueryShapeActorComponent();

  physx::PxRigidDynamic* GetPxActor() const { return m_pActor; }

protected:
  physx::PxRigidDynamic* m_pActor = nullptr;

private:
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
