#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct ezMsgComponentInternalTrigger;

struct ezSpawnComponentFlags
{
  using StorageType = ezUInt16;

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

using ezSpawnComponentManager = ezComponentManager<class ezSpawnComponent, ezBlockStorageType::Compact>;

/// \brief Spawns instances of prefabs dynamically at runtime.
///
/// The component may spawn prefabs automatically and also continuously, or it may only spawn objects on-demand
/// when triggered from code.
///
/// It keeps track of when it spawned an object and can ignore spawn requests that come in too early. Thus it can
/// also be used to take care of the logic that certain actions are only allowed every once in a while.
class EZ_GAMEENGINE_DLL ezSpawnComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpawnComponent, ezComponent, ezSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const ezVec3& vLocalOffset = ezVec3::MakeZero()); // [ scriptable ]

  /// \brief Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  /// \brief Enables that the component spawns right at creation time. Otherwise it needs to be triggered manually.
  void SetSpawnAtStart(bool b); // [ property ]
  bool GetSpawnAtStart() const; // [ property ]

  /// \brief Enables that once an object was spawned, another spawn action will be scheduled right away.
  void SetSpawnContinuously(bool b); // [ property ]
  bool GetSpawnContinuously() const; // [ property ]

  /// \brief Sets that spawned objects will be attached as child objects to this game object.
  void SetAttachAsChild(bool b);    // [ property ]
  bool GetAttachAsChild() const;    // [ property ]

  ezPrefabResourceHandle m_hPrefab; // [ property ]

  /// The minimum delay between spawning objects. This is also enforced for manually spawning things.
  ezTime m_MinDelay; // [ property ]

  /// For scheduled spawns (continuous / at start) this is an additional random range on top of the minimum spawn delay.
  ezTime m_DelayRange; // [ property ]

  /// The spawned object's orientation may deviate by this amount around the X axis. 180° is completely random orientation.
  ezAngle m_MaxDeviation;                                           // [ property ]

  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed parameter)

  /// Key/value pairs of parameters to pass to the prefab instantiation.
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;

protected:
  ezBitflags<ezSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const ezTransform& tLocalSpawn);
  bool SpawnOnce(const ezVec3& vLocalOffset);
  void OnTriggered(ezMsgComponentInternalTrigger& msg);

  ezTime m_LastManualSpawn;
};
