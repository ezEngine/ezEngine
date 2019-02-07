#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/World/World.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

typedef ezComponentManager<class ezPlayerStartPointComponent, ezBlockStorageType::Compact> ezPlayerStartPointComponentManager;

class EZ_GAMEENGINE_DLL ezPlayerStartPointComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPlayerStartPointComponent, ezComponent, ezPlayerStartPointComponentManager);

public:
  ezPlayerStartPointComponent();
  ~ezPlayerStartPointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // PROPERTIES
public:

  void SetPlayerPrefabFile(const char* szFile);
  const char* GetPlayerPrefabFile() const;

  void SetPlayerPrefab(const ezPrefabResourceHandle& hPrefab);
  const ezPrefabResourceHandle& GetPlayerPrefab() const;

  // TODO:
  // add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number
  // add prefab exposed properties


protected:


private:
  ezPrefabResourceHandle m_hPlayerPrefab;

};

