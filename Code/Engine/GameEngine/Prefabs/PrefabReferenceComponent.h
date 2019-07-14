#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Containers/ArrayMap.h>

class ezPrefabReferenceComponent;

class EZ_GAMEENGINE_DLL ezPrefabReferenceComponentManager : public ezComponentManager<ezPrefabReferenceComponent, ezBlockStorageType::Compact>
{
public:
  ezPrefabReferenceComponentManager(ezWorld* pWorld);
  ~ezPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

  void AddToUpdateList(ezPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

class EZ_GAMEENGINE_DLL ezPrefabReferenceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPrefabReferenceComponent, ezComponent, ezPrefabReferenceComponentManager);

public:
  ezPrefabReferenceComponent();
  ~ezPrefabReferenceComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPrefabFile(const char* szFile);
  const char* GetPrefabFile() const;

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);
  EZ_ALWAYS_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

  void InstantiatePrefab();

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  void ClearPreviousInstances();

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

  // ************************************* FUNCTIONS *****************************

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezPrefabResourceHandle m_hPrefab;
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
  bool m_bInUpdateList = false;
};

