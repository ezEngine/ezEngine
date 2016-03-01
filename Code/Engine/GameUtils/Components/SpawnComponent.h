#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Foundation/Time/Time.h>
#include <GameUtils/Prefabs/PrefabResource.h>

struct ezInputComponentMessage;

struct ezSpawnComponentFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    None = 0,
    SpawnAtStart = EZ_BIT(0),
    SpawnContinuously = EZ_BIT(1),
    AttachAsChild = EZ_BIT(2),

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
    StorageType SpawnContinuously : 1;
    StorageType AttachAsChild : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezSpawnComponentFlags);

typedef ezComponentManagerSimple<class ezSpawnComponent, true> ezSpawnComponentManager;

class EZ_GAMEUTILS_DLL ezSpawnComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpawnComponent, ezComponent, ezSpawnComponentManager);

public:
  ezSpawnComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPrefabFile(const char* szFile);
  const char* GetPrefabFile() const;

  bool GetSpawnAtStart() const { return m_Flags.IsAnySet(ezSpawnComponentFlags::SpawnAtStart); }
  void SetSpawnAtStart(bool b) { m_Flags.AddOrRemove(ezSpawnComponentFlags::SpawnAtStart, b); }

  bool GetSpawnContinuously() const { return m_Flags.IsAnySet(ezSpawnComponentFlags::SpawnContinuously); }
  void SetSpawnContinuously(bool b) { m_Flags.AddOrRemove(ezSpawnComponentFlags::SpawnContinuously, b); }

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);
  EZ_FORCE_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

protected:
  ezBitflags<ezSpawnComponentFlags> m_Flags;

  // ************************************* FUNCTIONS *****************************

  void InputComponentMessageHandler(ezInputComponentMessage& msg);

private:
  ezPrefabResourceHandle m_hPrefab;
};
