#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>

//////////////////////////////////////////////////////////////////////////

class ezWindowsSpatialAnchor;

typedef ezComponentManagerSimple<class ezMrSpatialAnchorComponent, ezComponentUpdateType::WhenSimulating> ezMrSpatialAnchorComponentManager;

class EZ_WINDOWSMIXEDREALITY_DLL ezMrSpatialAnchorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMrSpatialAnchorComponent, ezComponent, ezMrSpatialAnchorComponentManager);

public:
  ezMrSpatialAnchorComponent();
  ~ezMrSpatialAnchorComponent();

  //
  // ezComponent Interface
  //

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //
  // ezMrSpatialAnchorComponent Interface
  // 

public:
  /// \brief Attempts to create a new anchor at the given location.
  /// 
  /// On failure, the existing anchor will continue to be used.
  /// On success, the new anchor will be used and the new location will be persisted automatically,
  /// if the anchor has a persistence name.
  ezResult RecreateAnchorAt(const ezTransform& position);

  /// \brief Sets the unique name with which this anchor is identified.
  /// All anchors with a name will be persisted and restored when the app is launched again.
  /// 
  /// Set to null to turn this into a non-persistent anchor.
  void SetPersistentAnchorName(const char* szName);

  /// \brief Returns the name (or null) with which the anchor is persisted and restored.
  const char* GetPersistentAnchorName() const;

protected:
  void Update();
  void PersistCurrentLocation();
  ezResult RestorePersistedLocation();

  ezString m_sAnchorName;
  ezUniquePtr<ezWindowsSpatialAnchor> m_pSpatialAnchor;
};
