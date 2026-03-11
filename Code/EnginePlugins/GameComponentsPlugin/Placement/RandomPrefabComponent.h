#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class ezRandomPrefabComponent;

class EZ_GAMECOMPONENTS_DLL ezRandomPrefabComponentManager : public ezComponentManager<ezRandomPrefabComponent, ezBlockStorageType::Compact>
{
public:
  ezRandomPrefabComponentManager(ezWorld* pWorld);
  ~ezRandomPrefabComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
  void AddToUpdateList(ezRandomPrefabComponent* pComponent);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezHashSet<ezComponentHandle> m_ComponentsToUpdate;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Spawns one or multiple prefabs randomly from a collection.
///
/// The component stores a list of prefabs. Upon activation it randomly selects one or multiple to spawn.
/// The location, rotation, size and color may vary within specified limits.
///
/// The randomness is deterministic for each object, but different objects produce different results.
class EZ_GAMECOMPONENTS_DLL ezRandomPrefabComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRandomPrefabComponent, ezComponent, ezRandomPrefabComponentManager);

public:
  ezRandomPrefabComponent();
  ~ezRandomPrefabComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezRandomPrefabComponent

public:
  /// \brief Specifies how many objects to spawn.
  void SetCount(ezUInt16 uiCount);
  ezUInt16 GetCount() const;

  /// \brief Specifies how far along the x, y and z axis the spawned object positions may deviate from the center.
  void SetPositionDeviation(const ezVec3& vValue);
  const ezVec3& GetPositionDeviation() const { return m_vPositionDeviation; }

  /// \brief Specifies how much the spawned object rotations may deviate.
  void SetRotationDeviation(const ezVec3& vValue);
  const ezVec3& GetRotationDeviation() const { return m_vRotationDeviation; }

  /// \brief Sets the minimum scale for the spawned objects.
  void SetMinUniformScale(float fValue);
  float GetMinUniformScale() const { return m_fMinUniformScale; }

  /// \brief Sets the maximum scale for the spawned objects.
  void SetMaxUniformScale(float fValue);
  float GetMaxUniformScale() const { return m_fMaxUniformScale; }

  /// \brief If color1 and/or color2 are not white, objects are sent an ezMsgSetColor, with a color that is a random interpolation between the two.
  void SetColor1(const ezColor& value);
  const ezColor& GetColor1() const { return m_Color1; }

  /// \brief If color1 and/or color2 are not white, objects are sent an ezMsgSetColor, with a color that is a random interpolation between the two.
  void SetColor2(const ezColor& value);
  const ezColor& GetColor2() const { return m_Color2; }

  /// \brief If set to true, the spawned objects get attached to the owner of this component. Otherwise they don't have a parent.
  void SetInstantiateAsChildren(bool bValue);
  bool GetInstantiateAsChildren() const { return m_bInstantiateAsChildren; }

  /// \brief Whether to preview the result in editor. Can be disabled to reduce clutter or improve performance during editing.
  void SetPreview(bool bValue);
  bool GetPreview() const { return m_bPreview; }

private:
  void ClearCreatedInstances();
  void InstantiatePrefabs();

  ezUInt32 Prefabs_GetCount() const;
  ezString Prefabs_GetValue(ezUInt32 uiIndex) const;
  void Prefabs_SetValue(ezUInt32 uiIndex, ezString sValue);
  void Prefabs_Insert(ezUInt32 uiIndex, ezString sValue);
  void Prefabs_Remove(ezUInt32 uiIndex);

  ezUInt16 m_uiCount = 1;
  bool m_bInstantiateAsChildren = false;
  bool m_bPreview = true;

  ezVec3 m_vPositionDeviation = ezVec3(0);
  ezVec3 m_vRotationDeviation = ezVec3(0);
  float m_fMinUniformScale = 1.0f;
  float m_fMaxUniformScale = 1.0f;

  ezColor m_Color1 = ezColor::White;
  ezColor m_Color2 = ezColor::White;

  ezHybridArray<ezPrefabResourceHandle, 4> m_Prefabs;
};
