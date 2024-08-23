#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

struct ezMsgComponentInternalTrigger;

struct ezSpawnBoxComponentFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    None = 0,
    SpawnAtStart = EZ_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = EZ_BIT(1), ///< Every time a spawn duration has finished, a new one is started

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezSpawnBoxComponentFlags);

using ezSpawnBoxComponentManager = ezComponentManager<class ezSpawnBoxComponent, ezBlockStorageType::Compact>;

/// \brief This component spawns prefabs inside a box.
///
/// The prefabs are spawned over a fixed duration.
/// The number of prefabs to spawn over the time duration is randomly chosen.
/// Each prefab may get rotated around the Z axis and tilted away from the Z axis.
/// If desired, the component can start spawning automatically, or it can be (re-)started from code.
/// If 'spawn continuously' is enabled, the component restarts itself after the spawn duration is over,
/// thus for every spawn duration the number of prefabs to spawn gets reevaluated.
class ezSpawnBoxComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpawnBoxComponent, ezComponent, ezSpawnBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSpawnBoxComponent

public:
  /// \brief When called, the component starts spawning the chosen number of prefabs over the set duration.
  ///
  /// If this is called while the component is already active, the internal state is reset and it starts over.
  void StartSpawning();                                           // [ scriptable ]

  void SetHalfExtents(const ezVec3& value);                       // [ property ]
  const ezVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  bool GetSpawnAtStart() const;                                   // [ property ]
  void SetSpawnAtStart(bool b);                                   // [ property ]

  bool GetSpawnContinuously() const;                              // [ property ]
  void SetSpawnContinuously(bool b);                              // [ property ]

  ezTime m_SpawnDuration;                                         // [ property ]
  ezUInt16 m_uiMinSpawnCount = 5;                                 // [ property ]
  ezUInt16 m_uiSpawnCountRange = 5;                               // [ property ]
  ezPrefabResourceHandle m_hPrefab;                               // [ property ]

  /// The spawned object's forward direction may deviate this amount from the spawn box's forward rotation. This is accomplished by rotating around the Z axis.
  ezAngle m_MaxRotationZ; // [ property ]

  /// The spawned object's Z (up) axis may deviate by this amount from the spawn box's Z axis.
  ezAngle m_MaxTiltZ; // [ property ]


private:
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void Spawn(ezUInt32 uiCount);
  void InternalStartSpawning(bool bFirstTime);

  ezUInt16 m_uiSpawned = 0;
  ezUInt16 m_uiTotalToSpawn = 0;
  ezTime m_StartTime;
  ezBitflags<ezSpawnBoxComponentFlags> m_Flags;
  ezVec3 m_vHalfExtents = ezVec3(0.5f);
};
