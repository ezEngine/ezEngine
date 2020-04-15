#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>
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

  const ezRangeView<const char*, ezUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const ezVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, ezVariant& out_value) const; // [ property ] (exposed parameter)

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;

  // TODO:
  //  add properties to differentiate use cases, such as
  //  single player vs. multi-player spawn points
  //  team number

protected:
  ezPrefabResourceHandle m_hPlayerPrefab;
};
