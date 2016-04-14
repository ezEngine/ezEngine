#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameUtils/Prefabs/PrefabResource.h>

class ezPrefabReferenceComponent;

class EZ_GAMEUTILS_DLL ezPrefabReferenceComponentManager : public ezComponentManager<ezPrefabReferenceComponent>
{
public:
  ezPrefabReferenceComponentManager(ezWorld* pWorld);
  ~ezPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount);

  void AddToUpdateList(ezPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_PrefabComponentsToUpdate;
};

class EZ_GAMEUTILS_DLL ezPrefabReferenceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPrefabReferenceComponent, ezComponent, ezPrefabReferenceComponentManager);

public:
  ezPrefabReferenceComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPrefabFile(const char* szFile);
  const char* GetPrefabFile() const;

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);
  EZ_FORCE_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

  void InstantiatePrefab();

protected:

  // ************************************* FUNCTIONS *****************************

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezPrefabResourceHandle m_hPrefab;
  bool m_bRequiresInstantiation;
};
