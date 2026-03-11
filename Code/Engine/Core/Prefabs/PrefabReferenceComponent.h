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

  ezDeque<ezComponentHandle> m_ComponentsToUpdate;
};

/// \brief The central component to instantiate prefabs.
///
/// This component instantiates a prefab and attaches the instantiated objects as children to this object.
/// The component is able to remove and recreate instantiated objects, which is needed at editing time.
/// Whenever the prefab resource changes, this component re-creates the instance.
///
/// It also holds prefab parameters, which are passed through during instantiation.
/// For that it also implements remapping of game object references, so that they can be passed into prefabs during instantiation.
class EZ_CORE_DLL ezPrefabReferenceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPrefabReferenceComponent, ezComponent, ezPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);                                 // [ property ]
  EZ_ALWAYS_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; } // [ property ]

  void SetShowShapeIcons(bool bShow);                                                    // [ property ]
  bool GetShowShapeIcons() const;                                                        // [ property ]

  const ezRangeView<const char*, ezUInt32> GetParameters() const;                        // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const ezVariant& value);                          // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                                               // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, ezVariant& out_value) const;                      // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const ezWorld& world, ezWorldWriter& inout_stream, ezArrayMap<ezHashedString, ezVariant> parameters);
  static void DeserializePrefabParameters(ezArrayMap<ezHashedString, ezVariant>& out_parameters, ezWorldReader& inout_stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  ezPrefabResourceHandle m_hPrefab;
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};
