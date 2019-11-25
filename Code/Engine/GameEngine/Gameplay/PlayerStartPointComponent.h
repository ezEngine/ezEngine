#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

typedef ezComponentManager<class ezPlayerStartPointComponent, ezBlockStorageType::Compact> ezPlayerStartPointComponentManager;

class EZ_GAMEENGINE_DLL ezPlayerStartPointComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPlayerStartPointComponent, ezComponent, ezPlayerStartPointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayerStartPointComponent

public:
  ezPlayerStartPointComponent();
  ~ezPlayerStartPointComponent();

  void SetPlayerPrefabFile(const char* szFile); // [ property ]
  const char* GetPlayerPrefabFile() const;      // [ property ]

  void SetPlayerPrefab(const ezPrefabResourceHandle& hPrefab); // [ property ]
  const ezPrefabResourceHandle& GetPlayerPrefab() const;       // [ property ]

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number
  //  add prefab exposed properties

protected:
  ezPrefabResourceHandle m_hPlayerPrefab;
};
