#pragma once

#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Prefabs/PrefabResource.h>

struct ezMsgComponentInternalTrigger;

struct ezSpawnComponentFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    None = 0,
    SpawnAtStart = EZ_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = EZ_BIT(1), ///< Every time a scheduled spawn was done, a new one is scheduled
    AttachAsChild = EZ_BIT(2),     ///< All objects spawned will be attached as children to this node
    SpawnInFlight = EZ_BIT(3),     ///< [internal] A spawn trigger message has been posted.

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
    StorageType SpawnContinuously : 1;
    StorageType AttachAsChild : 1;
    StorageType SpawnInFlight : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezSpawnComponentFlags);

typedef ezComponentManager<class ezSpawnComponent, ezBlockStorageType::Compact> ezSpawnComponentManager;

class EZ_GAMEENGINE_DLL ezSpawnComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpawnComponent, ezComponent, ezSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSpawnComponent

public:
  ezSpawnComponent();
  ~ezSpawnComponent();

  /// \brief Checks whether the last spawn time was long enough ago that a call to TriggerManualSpawn() would succeed.
  bool CanTriggerManualSpawn() const; // [ scriptable ]

  /// \brief Spawns a new object, unless the minimum spawn delay has not been reached between calls to this function.
  ///
  /// Manual spawns and continuous (scheduled) spawns are independent from each other regarding minimum spawn delays.
  /// If this function is called in too short intervals, it is ignored and false is returned.
  /// Returns true, if an object was spawned.
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const ezVec3& vLocalOffset = ezVec3::ZeroVector()); // [ scriptable ]

  /// \brief Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  bool GetSpawnAtStart() const; // [ property ]
  void SetSpawnAtStart(bool b); // [ property ]

  bool GetSpawnContinuously() const; // [ property ]
  void SetSpawnContinuously(bool b); // [ property ]

  bool GetAttachAsChild() const; // [ property ]
  void SetAttachAsChild(bool b); // [ property ]

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);
  EZ_ALWAYS_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

  /// The minimum delay between spawning objects. This is also enforced for manually spawning things.
  ezTime m_MinDelay; // [ property ]

  /// For scheduled spawns (continuous / at start) this is an additional random range on top of the minimum spawn delay.
  ezTime m_DelayRange; // [ property ]

  /// The spawned object's orientation may deviate by this amount around the X axis. 180° is completely random orientation.
  ezAngle m_MaxDeviation; // [ property ]


protected:
  ezBitflags<ezSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const ezTransform& tLocalSpawn);
  bool SpawnOnce(const ezVec3& vLocalOffset);
  void OnTriggered(ezMsgComponentInternalTrigger& msg);

  ezTime m_LastManualSpawn;
  ezPrefabResourceHandle m_hPrefab;
};
