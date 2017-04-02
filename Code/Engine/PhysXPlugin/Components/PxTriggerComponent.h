#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

class EZ_PHYSXPLUGIN_DLL ezPxTriggerComponentManager : public ezComponentManager<class ezPxTriggerComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxTriggerComponentManager(ezWorld* pWorld);
  ~ezPxTriggerComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxTriggerComponent;

  void UpdateKinematicActors();

  ezDynamicArray<ezPxTriggerComponent*> m_KinematicActorComponents;
};

class EZ_PHYSXPLUGIN_DLL ezPxTriggerComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxTriggerComponent, ezPxActorComponent, ezPxTriggerComponentManager);

public:
  ezPxTriggerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

public:
  virtual void OnSimulationStarted() override;

  virtual void Deinitialize() override;

  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  //////////////////////////////////////////////////////////////////////////
  // Properties

  void SetKinematic(bool b);
  bool GetKinematic() const { return m_bKinematic; }

  void SetTriggerMessage(const char* sz) { m_sTriggerMessage.Assign(sz); }
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }
  ezHashedString m_sTriggerMessage;

protected:

  physx::PxRigidDynamic* m_pActor;

private:
  bool m_bKinematic = false;
  ezPxUserData m_UserData;
};
