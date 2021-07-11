#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

class ezPrefabReferenceComponent;

class EZ_CORE_DLL ezPrefabReferenceComponentManager : public ezComponentManager<ezPrefabReferenceComponent, ezBlockStorageType::Compact>
{
public:
  ezPrefabReferenceComponentManager(ezWorld* pWorld);
  ~ezPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
  void AddToUpdateList(ezPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezUInt32 m_uiGcStartIndex = 0;
  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

class EZ_CORE_DLL ezPrefabReferenceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPrefabReferenceComponent, ezComponent, ezPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPrefabReferenceComponent

public:
  ezPrefabReferenceComponent();
  ~ezPrefabReferenceComponent();

  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);                                 // [ property ]
  EZ_ALWAYS_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; } // [ property ]

  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const ezWorld& world, ezWorldWriter& stream, ezArrayMap<ezHashedString, ezVariant> parameters);
  static void DeserializePrefabParameters(ezArrayMap<ezHashedString, ezVariant>& out_parameters, ezWorldReader& stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  ezPrefabResourceHandle m_hPrefab;
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
  bool m_bInUpdateList = false;
};
